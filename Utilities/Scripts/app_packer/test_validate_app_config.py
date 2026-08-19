#!/usr/bin/env python3
"""Self-tests for validate_app_config.py.

Run directly; exits non-zero on the first failure. Stdlib only, like the script
it tests, so the apps-ci "prepare" job can run it with no pip install:

    python3 Utilities/Scripts/app_packer/test_validate_app_config.py

The pattern table is the point of this file. The dialect scanner is the part of
the validator whose failures are silent -- a construct that slips through does
not break CI, it ships a manifest that behaves differently on iOS and Android.
"""
import copy
import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import validate_app_config as v  # noqa: E402

failures = []


def check(name, condition, detail=""):
    if condition:
        print(f"ok:   {name}")
    else:
        print(f"FAIL: {name}" + (f" -- {detail}" if detail else ""))
        failures.append(name)


def dialect(pattern):
    """Errors reported for one pattern."""
    errors = v.Errors()
    v.check_pattern_dialect(pattern, "t", errors)
    return errors.items


# --- the pattern dialect ----------------------------------------------------

MUST_REJECT = [
    # anchors: matching is always a full match
    ("^abc", "leading anchor"),
    ("abc$", "trailing anchor"),
    # backreferences and lookaround
    (r"(a)\1", "backreference"),
    (r"(?=x)a", "lookahead"),
    (r"(?!x)a", "negative lookahead"),
    (r"(?<=x)a", "lookbehind"),
    (r"(?<n>a)", "named group"),
    (r"(?i)abc", "inline flags"),
    (r"(?>a)", "atomic group"),
    (r"\p{L}+", "unicode property"),
    (r"\A[a-z]+", r"\A anchor (literal in JavaScript)"),
    (r"[a-z]+\Z", r"\Z anchor (literal in JavaScript)"),
    # possessive quantifiers, on atoms *and* groups
    ("a++", "possessive on an atom"),
    ("[ab]++", "possessive on a class"),
    ("(a)++", "possessive on a group"),
    ("(?:a)++", "possessive on a non-capturing group"),
    ("(a){2,}+", "possessive on a braced quantifier"),
    # nested unbounded quantifiers, including behind extra parentheses
    ("(a+)+", "nested unbounded"),
    ("(a*)*", "nested unbounded, star"),
    ("((a+))+", "nested unbounded behind a group"),
    ("(?:(a+))*", "nested unbounded behind a non-capturing group"),
    ("((a+)b)+", "nested unbounded with a sibling"),
    ("((a+){2})+", "nested unbounded behind a bounded quantifier"),
    # cross-engine divergences
    ("[]]", "']' straight after '['"),
    ("a{,3}", "brace that is not a complete quantifier"),
    ("a\\", "trailing backslash"),
]

MUST_ACCEPT = [
    ("[A-Za-z0-9 ]+", "the Waypoint name pattern"),
    ("[^0-9]+", "negated class"),
    (r"\d{3}-\d{4}", "bounded quantifiers"),
    ("(cat|dog)s?", "alternation with optional"),
    (r"[a-z]+\.[a-z]{2,4}", "escaped dot"),
    ("(a|b){1,3}", "bounded quantifier on a group"),
    (r"\w+\s\w+", "letter escapes"),
    ("[A-Z]+[a-z]*", "two quantified atoms"),
    (r"a\{3\}", "escaped braces"),
    (r"x\]y", "escaped bracket"),
    ("[A-Za-z]+? ?[A-Za-z]*", "lazy quantifiers"),
    ("([A-Za-z]{1,3}){1,4}", "bounded nesting"),
]

for pattern, why in MUST_REJECT:
    check(f"rejects {pattern!r} ({why})", bool(dialect(pattern)))

for pattern, why in MUST_ACCEPT:
    errs = dialect(pattern)
    check(f"accepts {pattern!r} ({why})", not errs, str(errs))

# --- declarations -----------------------------------------------------------

GOOD = {
    "manifest_version": 1,
    "configFile": "app_config.json",
    "configFields": [
        {"id": "waypointName", "type": "string", "label": "Name",
         "description": "A name.", "default": "Waypoint",
         "minLength": 1, "maxLength": 16, "pattern": "[A-Za-z0-9 ]+"},
        {"id": "radiusM", "type": "int", "label": "Radius",
         "description": "A radius.", "default": 25, "min": 5, "max": 500,
         "unit": "m"},
        {"id": "lat", "type": "float", "label": "Latitude",
         "description": "A latitude.", "default": 51.5, "min": -90.0,
         "max": 90.0},
        {"id": "buzz", "type": "bool", "label": "Buzz",
         "description": "Whether to buzz.", "default": True},
    ],
}


def declare(mutate=None):
    """Validate a copy of GOOD, optionally mutated first."""
    manifest = copy.deepcopy(GOOD)
    if mutate:
        mutate(manifest)
    errors = v.Errors()
    v.validate_manifest_version(manifest, errors)
    v.validate_declaration(manifest, errors)
    return errors.items


def field(manifest, fid):
    return next(f for f in manifest["configFields"] if f["id"] == fid)


check("the reference declaration is clean", not declare(), str(declare()))

# manifest_version
check("rejects a missing manifest_version",
      bool(declare(lambda m: m.pop("manifest_version"))))
check("rejects manifest_version 2",
      bool(declare(lambda m: m.update(manifest_version=2))))
check("rejects manifest_version as a string",
      bool(declare(lambda m: m.update(manifest_version="1"))))
check("rejects manifest_version as a bool",
      bool(declare(lambda m: m.update(manifest_version=True))))

# configFile
check("rejects a configFile with a path",
      bool(declare(lambda m: m.update(configFile="sub/app_config.json"))))
check("rejects a configFile named app-manifest.json",
      bool(declare(lambda m: m.update(configFile="app-manifest.json"))))
check("rejects a configFile that is not .json",
      bool(declare(lambda m: m.update(configFile="app_config.txt"))))
check("rejects a 64-character configFile (the runtime refuses it)",
      bool(declare(lambda m: m.update(configFile="a" * 59 + ".json"))))
check("accepts a 63-character configFile",
      not declare(lambda m: m.update(configFile="a" * 58 + ".json")))
check("rejects configFields without configFile",
      bool(declare(lambda m: m.pop("configFile"))))
check("rejects configFile without configFields",
      bool(declare(lambda m: m.update(configFields=[]))))

# ids
check("rejects an id starting with a capital",
      bool(declare(lambda m: field(m, "radiusM").update(id="RadiusM"))))
check("rejects a duplicate id",
      bool(declare(lambda m: field(m, "radiusM").update(id="waypointName"))))
check("rejects ids differing only in case",
      bool(declare(lambda m: field(m, "radiusM").update(id="waypointname"))))

# per-type rules
check("rejects an int default outside min..max",
      bool(declare(lambda m: field(m, "radiusM").update(default=900))))
check("rejects max below min",
      bool(declare(lambda m: field(m, "radiusM").update(min=500, max=5))))
check("rejects a missing int max",
      bool(declare(lambda m: field(m, "radiusM").pop("max"))))
check("rejects a fractional int default",
      bool(declare(lambda m: field(m, "radiusM").update(default=25.5))))
check("rejects a bool default of 1",
      bool(declare(lambda m: field(m, "buzz").update(default=1))))
check("rejects min/max on a bool",
      bool(declare(lambda m: field(m, "buzz").update(min=0, max=1))))
check("rejects a float bound beyond binary32",
      bool(declare(lambda m: field(m, "lat").update(max=1e39))))
check("rejects a string default longer than maxLength",
      bool(declare(lambda m: field(m, "waypointName").update(
          default="an extremely long waypoint name"))))
check("rejects a string default failing its own pattern",
      bool(declare(lambda m: field(m, "waypointName").update(default="No!"))))
check("rejects maxLength over 128",
      bool(declare(lambda m: field(m, "waypointName").update(maxLength=200))))
check("rejects minLength greater than maxLength",
      bool(declare(lambda m: field(m, "waypointName").update(minLength=20))))
check("rejects a missing string maxLength",
      bool(declare(lambda m: field(m, "waypointName").pop("maxLength"))))
check("rejects unit on a string field",
      bool(declare(lambda m: field(m, "waypointName").update(unit="m"))))
check("rejects an unknown attribute",
      bool(declare(lambda m: field(m, "waypointName").update(placeholder="hi"))))
check("rejects a label over 32 characters",
      bool(declare(lambda m: field(m, "waypointName").update(label="x" * 33))))
check("rejects more than 32 fields",
      bool(declare(lambda m: m.update(configFields=[
          dict(GOOD["configFields"][3], id=f"flag{i}") for i in range(33)]))))
check("reports the worst-case values-file size even alongside other errors",
      any("bytes" in e for e in declare(lambda m: (
          m.update(configFields=[
              {"id": f"text{i}", "type": "string", "label": "L",
               "description": "D", "default": "x", "maxLength": 128}
              for i in range(32)]),
          m.update(configFile="app-manifest.json")))))

# --- the C++ field table ----------------------------------------------------

TABLE = '''
const AppConfig::Field kFields[] = {
    AppConfig::stringField("waypointName", "Waypoint", 1, 16),
    AppConfig::intField("radiusM", 25, 5, 500),
    AppConfig::floatField("lat", 51.5f, -90.0f, 90.0f),
    AppConfig::boolField("buzz", true),
};
constexpr const char *kFileName = "app_config.json";
'''


def bounds(table_source):
    """Cross-check a table source against the reference declaration."""
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "AppConfigFields.cpp"
        path.write_text(table_source, encoding="utf-8")
        errors = v.Errors()
        fields = v.validate_declaration(copy.deepcopy(GOOD), v.Errors())
        v.check_bounds(copy.deepcopy(GOOD), fields, [path], errors)
        return errors.items


check("the reference table matches the declaration", not bounds(TABLE),
      str(bounds(TABLE)))
check("catches an int bound that drifted",
      bool(bounds(TABLE.replace("25, 5, 500", "25, 5, 400"))))
check("catches a string default that drifted",
      bool(bounds(TABLE.replace('"Waypoint", 1, 16', '"Home", 1, 16'))))
check("catches a float bound that drifted",
      bool(bounds(TABLE.replace("-90.0f", "-80.0f"))))
check("accepts the exact binary32 round-trip of a declared float",
      not bounds(TABLE.replace("51.5f", "51.5000000f")))
check("catches a field missing from the table",
      bool(bounds(TABLE.replace(
          '    AppConfig::boolField("buzz", true),\n', ""))))
check("catches a field the declaration does not have",
      bool(bounds(TABLE.replace(
          "};", '    AppConfig::intField("bonus", 1, 0, 2),\n};'))))
check("catches the wrong factory for a declared type",
      bool(bounds(TABLE.replace('AppConfig::intField("radiusM", 25, 5, 500)',
                                'AppConfig::floatField("radiusM", 25.0f, 5.0f, 500.0f)'))))
check("rejects a named constant in place of a literal",
      bool(bounds(TABLE.replace("25, 5, 500", "25, 5, kMaxRadius"))))
check("ignores a commented-out entry",
      not bounds(TABLE.replace(
          "const AppConfig::Field",
          '// AppConfig::intField("ghost", 1, 0, 2),\nconst AppConfig::Field')))
check("survives a character literal containing a quote",
      not bounds("static char q = '\"';\n" + TABLE))
check("flags a configFile the sources never name",
      bool(bounds(TABLE.replace('"app_config.json"', '"other.json"'))))

# --- number formatting used by the size budget ------------------------------

check("float32 comparison treats adjacent values as different",
      not v.same_float(51.5072, 51.50721))
check("float32 comparison accepts an exact round-trip",
      v.same_float(51.5072, 51.507198333740234))

print()
if failures:
    print(f"{len(failures)} FAILED: {', '.join(failures)}")
    sys.exit(1)
print("all passed")
