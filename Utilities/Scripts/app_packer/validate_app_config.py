#!/usr/bin/env python3
"""Validate an app's configuration-field declaration, and the app's copy of it.

An app declares configuration fields in config.json ("configFile" and
"configFields"); the companion app collects values from the user and writes them
to the declared file next to the .uapp on the watch. The full contract is in
Docs/app-config-fields.md, and the machine-readable half is
app-config.schema.json next to this script.

This tool is the single implementation of the rules:

  --check CONFIG_JSON        validate the declaration in a config.json
  --check-bounds SOURCE      additionally compare the app's constexpr
                             SDK::AppConfig::Field table against that
                             declaration (repeatable)

It enforces everything the JSON Schema expresses, plus the rules a schema
cannot: unique ids, max >= min, a "default" that satisfies its own constraints,
the restricted regex dialect, byte-length limits, and the worst-case size of the
resulting values file.

Stdlib only, on purpose: the apps-ci "prepare" job runs in python:3.11-slim with
no pip install. If the optional `jsonschema` package happens to be importable it
is also run against app-config.schema.json, which keeps the schema file honest
during development.
"""
import argparse
import json
import re
import struct
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
SCHEMA_PATH = SCRIPT_DIR / "app-config.schema.json"

# --- Limits (Docs/app-config-fields.md section 8) ---------------------------

MAX_FIELDS = 32
MAX_ID_LEN = 32
MAX_LABEL_LEN = 32
MAX_DESCRIPTION_LEN = 200
MAX_MESSAGE_LEN = 120
MAX_UNIT_LEN = 8
MAX_STRING_BYTES = 128
MAX_PATTERN_LEN = 256
MAX_VALUES_FILE_BYTES = 8192

INT32_MIN = -(2 ** 31)
INT32_MAX = 2 ** 31 - 1
FLOAT32_MAX = 3.4028234663852886e38

# Longest plain-decimal text a conforming writer can emit for a binary32: a
# sign, up to 39 integer digits (binary32 tops out around 3.4e38), a point, and
# the 9 significant digits that round-trip the format. No exponent is allowed.
FLOAT_TEXT_MAX = 1 + 39 + 1 + 9

ID_RE = re.compile(r"^[a-z][A-Za-z0-9_]{0,31}$")
CONFIG_FILE_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.-]{0,57}\.json$")
RESERVED_FILE_NAMES = {"config.json"}

TYPES = ("string", "bool", "int", "float")

# The only letter/digit escapes the pattern dialect allows. Everything else that
# spells a letter after a backslash ('\A', '\Z', '\p', '\k', '\1', ...) either
# means different things across the three regex engines or is outright forbidden,
# so the scanner whitelists rather than blacklists.
ALLOWED_LETTER_ESCAPES = frozenset("dDwWsSbBnrt")

COMMON_KEYS = {"id", "type", "label", "description", "default", "required",
               "validationMessage"}
TYPE_KEYS = {
    "string": COMMON_KEYS | {"minLength", "maxLength", "pattern"},
    "bool": COMMON_KEYS,
    "int": COMMON_KEYS | {"min", "max", "unit"},
    "float": COMMON_KEYS | {"min", "max", "unit"},
}
TYPE_REQUIRED = {
    "string": {"id", "type", "label", "description", "default", "maxLength"},
    "bool": {"id", "type", "label", "description", "default"},
    "int": {"id", "type", "label", "description", "default", "min", "max"},
    "float": {"id", "type", "label", "description", "default", "min", "max"},
}


class Errors:
    """Collects every problem so one run reports them all."""

    def __init__(self):
        self.items = []

    def add(self, where, message):
        self.items.append(f"{where}: {message}")

    def __bool__(self):
        return bool(self.items)


def as_float32(value):
    """Round a Python float to the binary32 the watch will actually hold."""
    return struct.unpack("<f", struct.pack("<f", value))[0]


def is_int(value):
    # bool is a subclass of int in Python; JSON true must not pass as 1.
    return isinstance(value, int) and not isinstance(value, bool)


def is_number(value):
    return is_int(value) or isinstance(value, float)


# --- Regex dialect ---------------------------------------------------------

def check_pattern_dialect(pattern, where, errors):
    """Enforce the restricted dialect of Docs/app-config-fields.md section 3.2.

    Walks the pattern rather than pattern-matching it, so that a '^' inside a
    character class (negation, legal) is not confused with an anchor (illegal),
    and so a construct that appears after an escape is not misread.
    """
    i = 0
    n = len(pattern)
    in_class = False
    # Stack of "does the group body contain an unbounded quantifier": a
    # quantified group that itself quantifies is the classic catastrophic
    # backtracking shape, e.g. (a+)+.
    group_stack = []
    body_unbounded = False

    def flag(msg):
        errors.add(where, f"pattern: {msg}")

    while i < n:
        ch = pattern[i]

        if ch == "\\":
            if i + 1 >= n:
                flag("trailing backslash")
                break
            nxt = pattern[i + 1]
            if nxt.isdigit() and nxt != "0":
                flag(f"backreference '\\{nxt}' is not allowed")
            elif nxt == "k":
                flag("named backreference '\\k' is not allowed")
            elif nxt in "pP":
                flag(f"Unicode property escape '\\{nxt}{{...}}' is not allowed")
            elif nxt.isalnum() and nxt not in ALLOWED_LETTER_ESCAPES:
                # Whitelist, not blacklist: '\A' and '\Z' are anchors in Python,
                # Java and ICU but literal letters in JavaScript, so a pattern
                # using them matches different strings on iOS and Android. Any
                # letter escape outside the documented set is refused for the
                # same reason.
                flag(f"'\\{nxt}' is not one of the allowed escapes "
                     f"({', '.join('\\' + c for c in sorted(ALLOWED_LETTER_ESCAPES))})")
            i += 2
            continue

        if in_class:
            if ch == "]":
                in_class = False
            i += 1
            continue

        if ch == "[":
            in_class = True
            i += 1
            # A leading '^' negates.
            if i < n and pattern[i] == "^":
                i += 1
            if i < n and pattern[i] == "]":
                # Python and Java read this as a literal ']' inside the class;
                # JavaScript reads '[]' as an empty class that matches nothing,
                # then treats the ']' as a literal. Same pattern, different
                # matches, so require it escaped.
                flag("a ']' straight after '[' is a literal in some engines and "
                     "an empty class in others; write it as '\\]'")
                i += 1
            continue

        if ch in "^$":
            flag(f"'{ch}' is not allowed: matching is always a full match, "
                 f"so anchors are unnecessary")
            i += 1
            continue

        if ch == "(":
            if pattern.startswith("(?:", i):
                group_stack.append(body_unbounded)
                body_unbounded = False
                i += 3
                continue
            if pattern.startswith("(?", i):
                tail = pattern[i + 2:i + 4]
                if tail[:1] in ("=", "!"):
                    flag("lookahead is not allowed")
                elif tail.startswith("<") and tail[1:2] in ("=", "!"):
                    flag("lookbehind is not allowed")
                elif tail.startswith("<") or tail.startswith("P<"):
                    flag("named capture groups are not allowed")
                elif tail.startswith(">"):
                    flag("atomic groups are not allowed")
                else:
                    flag(f"inline group modifier '(?{tail}' is not allowed")
                # Skip the marker and keep scanning the body for more problems.
                group_stack.append(body_unbounded)
                body_unbounded = False
                i += 2
                continue
            group_stack.append(body_unbounded)
            body_unbounded = False
            i += 1
            continue

        if ch == ")":
            if not group_stack:
                flag("unbalanced ')'")
                i += 1
                continue
            inner_unbounded = body_unbounded
            enclosing_unbounded = group_stack.pop()
            i += 1
            quant, i = read_quantifier(pattern, i)
            if quant is not None and inner_unbounded and quant["unbounded"]:
                flag("a quantified group whose body is also unbounded "
                     "(such as '(a+)+') can backtrack exponentially; "
                     "rewrite it without the nesting")
            # The enclosing body inherits everything this group contained: an
            # unbounded quantifier does not stop counting just because a layer
            # of parentheses sits between it and the outer quantifier, or
            # '((a+))+' would slip through the check above.
            body_unbounded = (enclosing_unbounded or inner_unbounded or
                              (quant is not None and quant["unbounded"]))
            continue

        quant, after = read_quantifier(pattern, i)
        if quant is not None:
            if quant["possessive"]:
                flag("possessive quantifiers are not allowed")
            if quant["unbounded"]:
                body_unbounded = True
            i = after
            continue

        if ch == "{":
            # Not a well-formed {n} / {n,} / {n,m}. Python accepts '{,3}' as a
            # quantifier, JavaScript treats it as literal text and Java throws,
            # so an ambiguous brace cannot be allowed through.
            flag("'{' must open a complete {n}, {n,} or {n,m} quantifier; "
                 "write a literal brace as '\\{'")
            i += 1
            continue

        i += 1

    if in_class:
        errors.add(where, "pattern: unterminated character class '['")
    if group_stack:
        errors.add(where, "pattern: unbalanced '('")

    try:
        re.compile(pattern)
    except re.error as exc:
        errors.add(where, f"pattern: not a valid regular expression ({exc})")


def read_quantifier(pattern, i):
    """If a quantifier starts at i, describe it and return the index after it.

    Returns (None, i) when there is no quantifier at i. A lazy quantifier
    ('a+?') is fine -- every target engine agrees on it. A possessive one
    ('a++') is not.
    """
    n = len(pattern)
    if i >= n:
        return None, i
    ch = pattern[i]
    if ch in "*+?":
        end = i + 1
        unbounded = ch in "*+"
    elif ch == "{":
        match = re.match(r"\{(\d+)(,(\d*)?)?\}", pattern[i:])
        if not match:
            return None, i
        end = i + match.end()
        # {n,} has no upper bound; {n} and {n,m} do.
        unbounded = bool(match.group(2)) and not match.group(3)
    else:
        return None, i

    possessive = False
    if end < n and pattern[end] == "+":
        possessive = True
        end += 1
    elif end < n and pattern[end] == "?":
        end += 1  # lazy: allowed
    return {"unbounded": unbounded, "possessive": possessive}, end


def full_match(pattern, value):
    """Match the way the companion app must: anchored over the whole value."""
    try:
        return re.fullmatch(pattern, value) is not None
    except re.error:
        return False


# --- Declaration validation ------------------------------------------------

def check_text(field, key, limit, where, errors, required):
    value = field.get(key)
    if value is None:
        if required:
            errors.add(where, f"missing required '{key}'")
        return
    if not isinstance(value, str):
        errors.add(where, f"'{key}' must be a string")
        return
    if not value.strip():
        errors.add(where, f"'{key}' must not be empty")
        return
    if len(value) > limit:
        errors.add(where, f"'{key}' is {len(value)} characters, limit is {limit}")


def validate_field(field, index, errors, seen_ids):
    where = f"configFields[{index}]"
    if not isinstance(field, dict):
        errors.add(where, "must be an object")
        return None

    field_id = field.get("id")
    if not isinstance(field_id, str):
        errors.add(where, "missing required 'id' (a string)")
    elif not ID_RE.match(field_id):
        errors.add(where, f"id {field_id!r} must match {ID_RE.pattern} "
                          f"(lower-case first letter, then letters, digits or "
                          f"underscores, max {MAX_ID_LEN} characters)")
    else:
        where = f"configFields[{index}] '{field_id}'"
        # FAT is case-insensitive and so are tired reviewers: two ids differing
        # only in case are a bug waiting to happen.
        folded = field_id.lower()
        if folded in seen_ids:
            errors.add(where, f"duplicate id (already used as {seen_ids[folded]!r})")
        else:
            seen_ids[folded] = field_id

    field_type = field.get("type")
    if field_type not in TYPES:
        errors.add(where, f"'type' must be one of {', '.join(TYPES)}")
        return None

    unknown = set(field.keys()) - TYPE_KEYS[field_type]
    for key in sorted(unknown):
        errors.add(where, f"'{key}' is not a valid attribute of a "
                          f"'{field_type}' field")
    for key in sorted(TYPE_REQUIRED[field_type] - set(field.keys())):
        errors.add(where, f"missing required '{key}'")

    check_text(field, "label", MAX_LABEL_LEN, where, errors, required=False)
    check_text(field, "description", MAX_DESCRIPTION_LEN, where, errors, required=False)
    check_text(field, "validationMessage", MAX_MESSAGE_LEN, where, errors, required=False)
    check_text(field, "unit", MAX_UNIT_LEN, where, errors, required=False)

    if "required" in field and not isinstance(field["required"], bool):
        errors.add(where, "'required' must be true or false")

    if field_type == "bool":
        validate_bool_field(field, where, errors)
    elif field_type == "int":
        validate_int_field(field, where, errors)
    elif field_type == "float":
        validate_float_field(field, where, errors)
    else:
        validate_string_field(field, where, errors)

    return field


def validate_bool_field(field, where, errors):
    if "default" in field and not isinstance(field["default"], bool):
        errors.add(where, "'default' must be true or false")


def validate_int_field(field, where, errors):
    lo, hi = field.get("min"), field.get("max")
    for key, value in (("min", lo), ("max", hi)):
        if value is not None and not is_int(value):
            errors.add(where, f"'{key}' must be a whole number")
        elif is_int(value) and not (INT32_MIN <= value <= INT32_MAX):
            errors.add(where, f"'{key}' is outside the 32-bit signed range")
    if is_int(lo) and is_int(hi) and hi < lo:
        errors.add(where, f"'max' ({hi}) is less than 'min' ({lo})")

    default = field.get("default")
    if "default" in field and not is_int(default):
        errors.add(where, "'default' must be a whole number")
    elif is_int(default) and is_int(lo) and is_int(hi) and not (lo <= default <= hi):
        errors.add(where, f"'default' ({default}) is outside min..max ({lo}..{hi})")


def validate_float_field(field, where, errors):
    lo, hi = field.get("min"), field.get("max")
    for key, value in (("min", lo), ("max", hi)):
        if value is not None and not is_number(value):
            errors.add(where, f"'{key}' must be a number")
        elif is_number(value) and abs(value) > FLOAT32_MAX:
            errors.add(where, f"'{key}' is not representable as a "
                              f"single-precision float")
    if is_number(lo) and is_number(hi) and hi < lo:
        errors.add(where, f"'max' ({hi}) is less than 'min' ({lo})")

    default = field.get("default")
    if "default" in field and not is_number(default):
        errors.add(where, "'default' must be a number")
    elif is_number(default):
        if abs(default) > FLOAT32_MAX:
            errors.add(where, "'default' is not representable as a "
                              "single-precision float")
        elif is_number(lo) and is_number(hi) and not (lo <= default <= hi):
            errors.add(where, f"'default' ({default}) is outside min..max "
                              f"({lo}..{hi})")


def validate_string_field(field, where, errors):
    max_len = field.get("maxLength")
    min_len = field.get("minLength", 0)

    if "maxLength" in field:
        if not is_int(max_len) or not (1 <= max_len <= MAX_STRING_BYTES):
            errors.add(where, f"'maxLength' must be a whole number from 1 to "
                              f"{MAX_STRING_BYTES} (UTF-8 bytes)")
            max_len = None
    if "minLength" in field:
        if not is_int(min_len) or min_len < 0:
            errors.add(where, "'minLength' must be a whole number of 0 or more")
            min_len = None
        elif is_int(max_len) and min_len > max_len:
            errors.add(where, f"'minLength' ({min_len}) is greater than "
                              f"'maxLength' ({max_len})")

    pattern = field.get("pattern")
    if pattern is not None:
        if not isinstance(pattern, str) or not pattern:
            errors.add(where, "'pattern' must be a non-empty string")
            pattern = None
        elif len(pattern) > MAX_PATTERN_LEN:
            errors.add(where, f"'pattern' is {len(pattern)} characters, limit "
                              f"is {MAX_PATTERN_LEN}")
            pattern = None
        else:
            check_pattern_dialect(pattern, where, errors)

    default = field.get("default")
    if "default" in field and not isinstance(default, str):
        errors.add(where, "'default' must be a string")
        return
    if not isinstance(default, str):
        return

    default_bytes = len(default.encode("utf-8"))
    if is_int(max_len) and default_bytes > max_len:
        errors.add(where, f"'default' is {default_bytes} UTF-8 bytes, "
                          f"'maxLength' is {max_len}")
    if is_int(min_len) and default_bytes < min_len:
        errors.add(where, f"'default' is {default_bytes} UTF-8 bytes, "
                          f"'minLength' is {min_len}")
    if pattern and not full_match(pattern, default):
        errors.add(where, f"'default' ({default!r}) does not match its own "
                          f"'pattern' ({pattern!r}) as a full match")


def worst_case_values_size(fields):
    """Upper bound on the values file the companion app could write.

    Every declared id present, each holding the longest value it can. Numbers
    are bounded by the longest text a conforming writer can emit for them.
    """
    # {"schema":1,"values":{}} plus a comma between entries.
    size = len('{"schema":1,"values":{}}')
    for field in fields:
        field_type = field.get("type")
        field_id = field.get("id", "")
        size += len(json.dumps(field_id)) + 2  # "id":
        if field_type == "string":
            max_len = field.get("maxLength", MAX_STRING_BYTES)
            max_len = max_len if is_int(max_len) else MAX_STRING_BYTES
            # A conforming writer emits UTF-8 directly and escapes only '"',
            # '\' and control characters, so a byte costs at most two.
            size += 2 + max_len * 2
        elif field_type == "bool":
            size += len("false")
        elif field_type == "int":
            size += len(str(INT32_MIN))
        else:
            size += FLOAT_TEXT_MAX
        size += 1  # comma
    return size


def validate_declaration(config, errors):
    fields = config.get("configFields")
    config_file = config.get("configFile")

    if fields is None:
        if config_file is not None:
            errors.add("config.json", "'configFile' is set but no "
                                      "'configFields' are declared")
        return []

    if not isinstance(fields, list):
        errors.add("config.json", "'configFields' must be an array")
        return []

    if not fields:
        if config_file is not None:
            errors.add("config.json", "'configFile' is set but 'configFields' "
                                      "is empty")
        return []

    if len(fields) > MAX_FIELDS:
        errors.add("config.json", f"{len(fields)} configuration fields "
                                  f"declared, limit is {MAX_FIELDS}")

    if config_file is None:
        errors.add("config.json", "'configFields' is declared but "
                                  "'configFile' is missing: the companion app "
                                  "needs a filename to write")
    elif not isinstance(config_file, str):
        errors.add("config.json", "'configFile' must be a string")
    else:
        if not CONFIG_FILE_RE.match(config_file):
            errors.add("config.json", f"'configFile' {config_file!r} must be a "
                                      f"bare filename ending in .json with no "
                                      f"path separators "
                                      f"(pattern {CONFIG_FILE_RE.pattern})")
        if config_file.lower() in RESERVED_FILE_NAMES:
            errors.add("config.json", f"'configFile' must not be "
                                      f"{config_file!r}: that name is reserved "
                                      f"for the package metadata, which is "
                                      f"never copied to the watch")

    seen_ids = {}
    validated = [f for f in (validate_field(f, i, errors, seen_ids)
                             for i, f in enumerate(fields)) if f]

    if validated and not errors:
        worst = worst_case_values_size(validated)
        if worst > MAX_VALUES_FILE_BYTES:
            errors.add("config.json",
                       f"these fields could produce a values file of up to "
                       f"{worst} bytes, over the {MAX_VALUES_FILE_BYTES}-byte "
                       f"limit; shorten the string fields' 'maxLength' or "
                       f"declare fewer fields")

    return validated


def run_optional_schema_check(config, errors):
    """Cross-check against the published JSON Schema when jsonschema is around.

    Not required (CI has no pip install) but it catches the schema file drifting
    away from the rules implemented here.
    """
    try:
        import jsonschema  # type: ignore
    except ImportError:
        return False
    try:
        schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    except OSError as exc:
        errors.add(SCHEMA_PATH.name, f"could not be read ({exc})")
        return False
    validator = jsonschema.Draft202012Validator(schema)
    for problem in sorted(validator.iter_errors(config), key=lambda e: e.path):
        location = "/".join(str(p) for p in problem.absolute_path) or "config.json"
        errors.add(f"schema:{location}", problem.message)
    return True


# --- Cross-checking the app's field table ----------------------------------

FACTORY_RE = re.compile(
    r"(?:SDK\s*::\s*)?(?:AppConfig\s*::\s*)?"
    r"(string|bool|int|float)Field\s*\(")
# Any ".json" string literal in the checked sources. The values file is opened
# by name somewhere in the app -- at the constructor, or via a constant like
# kFileName -- so requiring the declared name to appear as a literal catches an
# app that opens a file its config.json never declared, without depending on how
# the call happens to be written.
JSON_LITERAL_RE = re.compile(r"\"([A-Za-z0-9_.-]+\.json)\"")


def strip_comments(source):
    """Remove // and /* */ comments so a commented-out entry is not parsed."""
    out = []
    i = 0
    n = len(source)
    while i < n:
        two = source[i:i + 2]
        if two == "//":
            end = source.find("\n", i)
            i = n if end < 0 else end
        elif two == "/*":
            end = source.find("*/", i + 2)
            i = n if end < 0 else end + 2
        elif source[i] == '"':
            j = i + 1
            while j < n:
                if source[j] == "\\":
                    j += 2
                    continue
                if source[j] == '"':
                    j += 1
                    break
                j += 1
            out.append(source[i:j])
            i = j
        else:
            out.append(source[i])
            i += 1
    return "".join(out)


def split_arguments(text, start):
    """Split the argument list that opens at `start` (just past the '(')."""
    args = []
    current = []
    depth = 0
    i = start
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == '"':
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    j += 1
                    break
                j += 1
            current.append(text[i:j])
            i = j
            continue
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            if depth == 0:
                args.append("".join(current).strip())
                return [a for a in args if a != ""], i + 1
            depth -= 1
        elif ch == "," and depth == 0:
            args.append("".join(current).strip())
            current = []
            i += 1
            continue
        current.append(ch)
        i += 1
    return None, i


CPP_STRING_ESCAPES = {"n": "\n", "t": "\t", "r": "\r", '"': '"',
                      "\\": "\\", "0": "\0", "'": "'"}


def parse_cpp_string(literal):
    if len(literal) < 2 or literal[0] != '"' or literal[-1] != '"':
        return None
    body = literal[1:-1]
    out = []
    i = 0
    while i < len(body):
        if body[i] == "\\" and i + 1 < len(body):
            nxt = body[i + 1]
            if nxt not in CPP_STRING_ESCAPES:
                return None  # \x41, \u... : not worth guessing at
            out.append(CPP_STRING_ESCAPES[nxt])
            i += 2
            continue
        out.append(body[i])
        i += 1
    return "".join(out)


def parse_cpp_int(literal):
    text = literal.replace(" ", "").replace("'", "")
    text = re.sub(r"[uUlL]+$", "", text)
    try:
        return int(text, 0)
    except ValueError:
        return None


def parse_cpp_float(literal):
    text = literal.replace(" ", "").replace("'", "")
    text = re.sub(r"[fFlL]$", "", text)
    try:
        return float(text)
    except ValueError:
        return None


def parse_cpp_bool(literal):
    text = literal.strip()
    if text == "true":
        return True
    if text == "false":
        return False
    return None


def parse_field_table(path, errors):
    """Extract the SDK::AppConfig::Field entries from a C++ source file.

    Returns (entries, json_names): the field table keyed by id, and every
    ".json" string literal the file contains.
    """
    try:
        source = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
    except OSError as exc:
        errors.add(str(path), f"could not be read ({exc})")
        return {}, None

    entries = {}
    for match in FACTORY_RE.finditer(source):
        kind = match.group(1)
        args, _ = split_arguments(source, match.end())
        where = f"{path.name} {kind}Field(...)"
        if args is None:
            errors.add(where, "unterminated argument list")
            continue

        field_id = parse_cpp_string(args[0]) if args else None
        if field_id is None:
            errors.add(where, "the first argument must be a plain string "
                              "literal id so CI can check it")
            continue
        where = f"{path.name} '{field_id}'"

        expected = {"string": 4, "bool": 2, "int": 4, "float": 4}[kind]
        if len(args) != expected:
            errors.add(where, f"{kind}Field takes {expected} arguments, "
                              f"found {len(args)}")
            continue

        entry = {"type": kind}
        if kind == "bool":
            entry["default"] = parse_cpp_bool(args[1])
            names = ("default",)
            values = (entry["default"],)
        elif kind == "int":
            entry["default"] = parse_cpp_int(args[1])
            entry["min"] = parse_cpp_int(args[2])
            entry["max"] = parse_cpp_int(args[3])
            names = ("default", "min", "max")
            values = (entry["default"], entry["min"], entry["max"])
        elif kind == "float":
            entry["default"] = parse_cpp_float(args[1])
            entry["min"] = parse_cpp_float(args[2])
            entry["max"] = parse_cpp_float(args[3])
            names = ("default", "min", "max")
            values = (entry["default"], entry["min"], entry["max"])
        else:
            entry["default"] = parse_cpp_string(args[1])
            entry["minLength"] = parse_cpp_int(args[2])
            entry["maxLength"] = parse_cpp_int(args[3])
            names = ("default", "minLength", "maxLength")
            values = (entry["default"], entry["minLength"], entry["maxLength"])

        bad = [name for name, value in zip(names, values) if value is None]
        if bad:
            errors.add(where, f"could not read {', '.join(bad)} from the "
                              f"table; use plain literals (not named constants "
                              f"or expressions) so CI can compare them")
            continue

        if field_id in entries:
            errors.add(where, "declared twice in the field table")
            continue
        entries[field_id] = entry

    return entries, set(JSON_LITERAL_RE.findall(source))


def same_float(declared, in_code):
    """Compare as the binary32 the app will hold, not as Python doubles."""
    return as_float32(float(declared)) == as_float32(float(in_code))


def check_bounds(config, declared_fields, sources, errors):
    table = {}
    json_names = set()
    for path in sources:
        entries, found_names = parse_field_table(path, errors)
        for field_id, entry in entries.items():
            if field_id in table:
                errors.add(str(path), f"'{field_id}' also appears in another "
                                      f"source file")
                continue
            table[field_id] = entry
        json_names |= found_names

    declared_by_id = {f["id"]: f for f in declared_fields if isinstance(f.get("id"), str)}

    for field_id in sorted(set(declared_by_id) - set(table)):
        errors.add("field table", f"'{field_id}' is declared in config.json but "
                                  f"missing from the app's field table")
    for field_id in sorted(set(table) - set(declared_by_id)):
        errors.add("field table", f"'{field_id}' is in the app's field table "
                                  f"but not declared in config.json")

    for field_id in sorted(set(declared_by_id) & set(table)):
        declared = declared_by_id[field_id]
        in_code = table[field_id]
        where = f"field table '{field_id}'"

        if declared["type"] != in_code["type"]:
            errors.add(where, f"declared as '{declared['type']}' in config.json "
                              f"but built as {in_code['type']}Field")
            continue

        if in_code["type"] == "float":
            comparisons = (("default", same_float), ("min", same_float),
                           ("max", same_float))
        elif in_code["type"] == "int":
            comparisons = (("default", lambda a, b: a == b),
                           ("min", lambda a, b: a == b),
                           ("max", lambda a, b: a == b))
        elif in_code["type"] == "bool":
            comparisons = (("default", lambda a, b: a == b),)
        else:
            comparisons = (("default", lambda a, b: a == b),
                           ("minLength", lambda a, b: a == b),
                           ("maxLength", lambda a, b: a == b))

        for key, equal in comparisons:
            expected = declared.get(key, 0 if key == "minLength" else None)
            actual = in_code.get(key)
            if expected is None or actual is None:
                continue
            if not equal(expected, actual):
                errors.add(where, f"{key} is {actual!r} in the field table but "
                                  f"{expected!r} in config.json")

    config_file = config.get("configFile")
    if json_names and isinstance(config_file, str) and config_file not in json_names:
        errors.add("field table",
                   f"config.json declares configFile {config_file!r}, but the "
                   f"checked sources only name "
                   f"{', '.join(repr(n) for n in sorted(json_names))}")

    return table


# --- Entry point -----------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description="Validate an app's configuration-field declaration.")
    ap.add_argument("--check", metavar="CONFIG_JSON", required=True,
                    help="config.json to validate")
    ap.add_argument("--check-bounds", metavar="SOURCE", action="append",
                    default=[],
                    help="C++ source holding the app's SDK::AppConfig::Field "
                         "table, to cross-check against the declaration "
                         "(repeatable)")
    args = ap.parse_args()

    path = Path(args.check)
    try:
        config = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        sys.exit(f"error: {path}: could not be read ({exc})")
    except json.JSONDecodeError as exc:
        sys.exit(f"error: {path}: not valid JSON ({exc})")
    if not isinstance(config, dict):
        sys.exit(f"error: {path}: top level must be an object")

    errors = Errors()
    fields = validate_declaration(config, errors)
    schema_checked = run_optional_schema_check(config, errors)

    table = {}
    if args.check_bounds:
        if not fields and not errors:
            print(f"{path}: no configuration fields declared; nothing to "
                  f"cross-check")
        else:
            table = check_bounds(config, fields, [Path(s) for s in args.check_bounds],
                                 errors)

    if errors:
        print(f"error: {path}: {len(errors.items)} problem(s) with the "
              f"configuration declaration:", file=sys.stderr)
        for item in errors.items:
            print(f"  {item}", file=sys.stderr)
        sys.exit(1)

    if not fields:
        print(f"{path}: no configuration fields declared OK")
        return

    summary = f"{path}: {len(fields)} configuration field(s) OK"
    if schema_checked:
        summary += ", schema OK"
    if args.check_bounds:
        summary += f", field table matches ({len(table)} entries)"
    print(summary)


if __name__ == "__main__":
    main()
