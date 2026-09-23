#!/usr/bin/env python3
"""Check that a narrowed font can still render the runtime text put through it.

Glyph bitmaps are resident RAM on the watch: Sections.ld puts the font tables
inside .text and there is one RAM region, so every glyph a typography carries
costs image RAM whether or not it is ever drawn. Narrowing WildcardCharacters
from the full 95-character printable ASCII set down to the characters an app
actually substitutes is worth tens of kilobytes on the largest fonts.

It is also silent when it goes wrong: a character with no glyph is drawn as
FallbackCharacter, on one screen, with nothing to see at build time.

WHAT WildcardCharacters ACTUALLY CONTROLS
-----------------------------------------
Only the wildcard. Characters that appear in a Text's own translation are
built into the font regardless - ClockfaceSmile declares "0123456789-" for
Poppins_SemiBold_60 and its generated table still carries ':' for the static
TEXT_SMILE_COLON. FallbackCharacter is added for you too; Alarm declares
"0123456789:" and still gets '?'.

So the set needs to cover exactly one thing: what CODE writes into a wildcard
buffer at runtime. That is invisible to this tool and to the Designer file,
which is why an app declares it:

    <app>/assets/texts/wildcard-requirements.json

      {
        "Poppins_SemiBold_60": {
          "why":            "IntervalsTimer writes \"%s\" of T_TEXT_OPEN here",
          "rendersTextIds": ["TEXT_OPEN"]
        }
      }

The check is then: every character of every declared id's translations must be
in that typography's WildcardCharacters. That makes a translation edit, or a
new language, a build failure instead of a '?' on the watch.

A SPACE is the easiest one to get wrong: it is not added for you, and any
"%s: %u" style format puts one through the wildcard.

  --check TEXTS_XML       an app's assets/texts/texts.xml (repeatable)
  --require-manifest      every narrowed typography in these files must be
                          declared, so an app that depends on a declaration
                          cannot quietly lose it

WHAT THIS DOES NOT DO: it cannot see a new code path. If someone writes a
different string into one of these widgets tomorrow, only the declaration
keeps it honest. The manifest is a record of a human decision, checked for
consistency - not a substitute for making it.

Stdlib only, on purpose: the apps-ci "prepare" job runs in python:3.11-slim
with no pip install.
"""
import argparse
import json
import os
import re
import sys
import xml.etree.ElementTree as ET

# TouchGFX wildcard placeholder in a translation: the literal characters are
# not drawn, whatever the widget is filled with is.
PLACEHOLDER = re.compile(r"<\d*>")

PRINTABLE_ASCII = set(chr(c) for c in range(0x20, 0x7F))

MANIFEST_NAME = "wildcard-requirements.json"


def parse_texts(path):
    root = ET.parse(path).getroot()
    typos = {t.get("Id"): t for t in root.iter("Typography")}
    texts = {}
    for t in root.iter("Text"):
        texts[t.get("Id")] = [(tr.text or "") for tr in t.iter("Translation")]
    return typos, texts


def rendered_chars(translations):
    out = set()
    for tr in translations:
        out |= set(PLACEHOLDER.sub("", tr))
    return out


def describe(chars):
    return " ".join(("SPACE" if c == " " else c) for c in sorted(chars))


def check_file(path, require_manifest=False):
    try:
        typos, texts = parse_texts(path)
    except (OSError, ET.ParseError) as exc:
        return ["%s: could not be read (%s)" % (path, exc)]

    manifest_path = os.path.join(os.path.dirname(path), MANIFEST_NAME)
    manifest = {}
    if os.path.isfile(manifest_path):
        try:
            with open(manifest_path, encoding="utf-8") as f:
                manifest = json.load(f)
        except (OSError, ValueError) as exc:
            return ["%s: not valid JSON (%s)" % (manifest_path, exc)]
        if not isinstance(manifest, dict):
            return ["%s: top level must be an object" % manifest_path]

    errors = []
    for name, elem in sorted(typos.items()):
        wc = elem.get("WildcardCharacters")
        if wc is None:
            continue
        allowed = set(wc)
        if PRINTABLE_ASCII.issubset(allowed):
            continue                      # not narrowed, nothing to protect

        entry = manifest.get(name)
        if entry is None:
            if require_manifest:
                errors.append(
                    "%s: typography '%s' is narrowed but is not declared in %s.\n"
                    "    This app is checked with --require-manifest because it "
                    "has a coupling this tool cannot see. Add an entry naming "
                    "the Text ids whose translations are written into its "
                    "wildcards at runtime (an empty list is fine, with a 'why')."
                    % (path, name, MANIFEST_NAME))
            continue

        if not entry.get("why"):
            errors.append("%s: '%s' needs a 'why' naming the code path that "
                          "renders the declared ids" % (manifest_path, name))

        declared = entry.get("rendersTextIds", [])
        if not isinstance(declared, list):
            errors.append("%s: '%s'.rendersTextIds must be a list"
                          % (manifest_path, name))
            continue

        required = {}
        for tid in declared:
            if tid not in texts:
                errors.append("%s: '%s' declares Text id '%s', which does not "
                              "exist in %s" % (manifest_path, name, tid, path))
                continue
            for c in rendered_chars(texts[tid]):
                required.setdefault(c, tid)

        missing = sorted(set(required) - allowed)
        if missing:
            detail = "; ".join("%s (from %s)" % (describe([c]), required[c])
                               for c in missing)
            errors.append(
                "%s: typography '%s' cannot render: %s\n"
                "    Those characters go through its wildcard and have no "
                "glyph, so they draw as the fallback. Add them to "
                "WildcardCharacters." % (path, name, detail))

    return errors


def main():
    ap = argparse.ArgumentParser(
        description="Check narrowed font wildcard sets against declared runtime text.")
    ap.add_argument("--check", metavar="TEXTS_XML", action="append", required=True)
    ap.add_argument("--require-manifest", action="store_true")
    args = ap.parse_args()

    errors = []
    for path in args.check:
        errors += check_file(path, require_manifest=args.require_manifest)

    if errors:
        for e in errors:
            print("error: %s" % e, file=sys.stderr)
        sys.exit(1)

    print("wildcard sets OK (%d file%s checked)"
          % (len(args.check), "" if len(args.check) == 1 else "s"))


if __name__ == "__main__":
    main()
