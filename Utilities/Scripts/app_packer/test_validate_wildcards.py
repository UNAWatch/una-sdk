#!/usr/bin/env python3
"""Self-test for validate_wildcards.py.

A check that cannot fail is worse than no check, so most of these assert a
FAILURE and name the scenario it stands for. Run from anywhere; stdlib only.
"""
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import validate_wildcards as vw  # noqa: E402

TEXTS = """<?xml version="1.0" encoding="UTF-8"?>
<TextDatabase>
  <Texts>
    <TextGroup Id="All">
      <Text Id="WC_VALUE" Alignment="Center" TypographyId="Narrow">
        <Translation Language="GB">&lt;&gt;</Translation>
      </Text>
      <Text Id="STATIC_COLON" Alignment="Center" TypographyId="Narrow">
        <Translation Language="GB">:</Translation>
      </Text>
      <Text Id="RUNTIME_WORD" Alignment="Left" TypographyId="Wide">
        <Translation Language="GB">%s</Translation>
      </Text>
    </TextGroup>
  </Texts>
  <Typographies>
    <Typography Id="Narrow" Font="F.ttf" Size="60" Bpp="4" Direction="LTR"
        FallbackCharacter="?" WildcardCharacters="%s" />
    <Typography Id="Wide" Font="F.ttf" Size="18" Bpp="4" Direction="LTR"
        FallbackCharacter="?" WildcardCharacters="%s" />
  </Typographies>
</TextDatabase>
"""

FULL_ASCII = "".join(chr(c) for c in range(0x20, 0x7F))

CASES = []


def case(name):
    def deco(fn):
        CASES.append((name, fn))
        return fn
    return deco


def xml_attr(s):
    """Escape for an XML attribute - FULL_ASCII contains & < > and a quote."""
    return (s.replace("&", "&amp;").replace("<", "&lt;")
             .replace(">", "&gt;").replace('"', "&quot;"))


def build(tmp, wildcard, runtime_word="Open", manifest=None, wide=FULL_ASCII):
    path = os.path.join(tmp, "texts.xml")
    with open(path, "w", encoding="utf-8") as f:
        f.write(TEXTS % (xml_attr(runtime_word), xml_attr(wildcard), xml_attr(wide)))
    if manifest is not None:
        with open(os.path.join(tmp, vw.MANIFEST_NAME), "w", encoding="utf-8") as f:
            json.dump(manifest, f)
    return path


DECLARED = {"Narrow": {"why": "code writes T_RUNTIME_WORD here",
                       "rendersTextIds": ["RUNTIME_WORD"]}}


@case("declared runtime text that fits passes")
def _(tmp):
    p = build(tmp, "0123456789:Open", manifest=DECLARED)
    return vw.check_file(p) == []


@case("a translation gaining a character fails")
def _(tmp):
    # The language/wording change this whole check exists for.
    p = build(tmp, "0123456789:Open", runtime_word="Opened", manifest=DECLARED)
    errs = vw.check_file(p)
    return len(errs) == 1 and "'d'" in errs[0].replace('"', "'") or \
        (len(errs) == 1 and " d " in errs[0])


@case("a missing SPACE fails - the easiest one to get wrong")
def _(tmp):
    p = build(tmp, "0123456789:Open", runtime_word="Open Now", manifest=DECLARED)
    errs = vw.check_file(p)
    return len(errs) == 1 and "SPACE" in errs[0]


@case("a lost manifest fails under --require-manifest")
def _(tmp):
    p = build(tmp, "0123456789:Open")          # no manifest written
    return vw.check_file(p) == [] and len(vw.check_file(p, require_manifest=True)) == 1


@case("static translation text needs no entry - the converter adds it")
def _(tmp):
    # ':' is only in a static translation, never in the wildcard set.
    p = build(tmp, "0123456789Open", manifest=DECLARED)
    return vw.check_file(p) == []


@case("FallbackCharacter needs no entry - the converter adds it")
def _(tmp):
    p = build(tmp, "0123456789:Open", manifest=DECLARED)
    return vw.check_file(p) == []


@case("a full-ASCII typography is never constrained")
def _(tmp):
    p = build(tmp, FULL_ASCII, runtime_word="anything at all", manifest={})
    return vw.check_file(p, require_manifest=True) == []


@case("declaring a Text id that does not exist fails")
def _(tmp):
    m = {"Narrow": {"why": "x", "rendersTextIds": ["NO_SUCH_ID"]}}
    p = build(tmp, "0123456789:", manifest=m)
    errs = vw.check_file(p)
    return len(errs) == 1 and "does not exist" in errs[0]


@case("a declaration without a 'why' fails")
def _(tmp):
    m = {"Narrow": {"rendersTextIds": []}}
    p = build(tmp, "0123456789:", manifest=m)
    errs = vw.check_file(p)
    return len(errs) == 1 and "why" in errs[0]


@case("a declaration for a typography that no longer exists fails")
def _(tmp):
    # A renamed font would otherwise orphan its declaration silently.
    m = {"Poppins_Gone_60": {"why": "x", "rendersTextIds": []},
         "Narrow": {"why": "y", "rendersTextIds": []}}
    p = build(tmp, "0123456789:", manifest=m)
    errs = vw.check_file(p)
    return len(errs) == 1 and "Poppins_Gone_60" in errs[0]


@case("a declaration for a font that is no longer narrowed fails")
def _(tmp):
    # Widening a set back to full ASCII should not leave a stale declaration
    # sitting there looking like protection.
    m = {"Wide": {"why": "x", "rendersTextIds": []},
         "Narrow": {"why": "y", "rendersTextIds": []}}
    p = build(tmp, "0123456789:", manifest=m)
    errs = vw.check_file(p)
    return len(errs) == 1 and "not narrowed" in errs[0]


@case("a '_note' key in the manifest is not treated as a typography")
def _(tmp):
    m = {"_note": "free text for humans",
         "Narrow": {"why": "y", "rendersTextIds": []}}
    p = build(tmp, "0123456789:", manifest=m)
    return vw.check_file(p) == []


@case("a named <value> placeholder is not mistaken for text")
def _(tmp):
    # TouchGFX allows <>, <1> and <name>; only the substitution is drawn.
    # Workout uses the named form, so treating it as literal would demand
    # glyphs for '<', 'v', 'a', 'l', 'u', 'e', '>' that nothing renders.
    p = build(tmp, "0123456789:", runtime_word="<value>", manifest=DECLARED)
    return vw.check_file(p) == []


@case("a declaration that loses rendersTextIds fails, rather than checking nothing")
def _(tmp):
    # The hole this tool exists to close: with the key absent, defaulting to
    # [] would leave the declaration present and the check vacuous.
    m = {"Narrow": {"why": "code writes T_RUNTIME_WORD here"}}
    p = build(tmp, "0123456789:", manifest=m)
    errs = vw.check_file(p)
    return len(errs) == 1 and "rendersTextIds" in errs[0]


def main():
    failed = 0
    for name, fn in CASES:
        with tempfile.TemporaryDirectory() as tmp:
            try:
                ok = fn(tmp)
            except Exception as exc:                      # noqa: BLE001
                ok, name = False, "%s (raised %r)" % (name, exc)
        print("%s  %s" % ("PASS" if ok else "FAIL", name))
        failed += 0 if ok else 1

    if failed:
        print("\n%d of %d cases failed" % (failed, len(CASES)), file=sys.stderr)
        sys.exit(1)
    print("\nall %d cases passed" % len(CASES))


if __name__ == "__main__":
    main()
