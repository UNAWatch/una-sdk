#!/usr/bin/env python3
"""Ratcheting compiler-warning gate for the gcc simulator builds.

The simulator build already asks for the warnings we want: every app's una/Makefile
compiles with -Wall -Wextra -Wformat=2 -Wcast-qual and more. Two things then throw
the answers away -- una/Makefile appends -Wno-error to user_cflags after asking for
-Werror, and linux-simulator.yml only ever tees the log into an uploaded artifact
that nothing reads.

So the diagnostics are printed and ignored. #247 hand-fixed a printf("%d", size_t)
in Mock/AppMemory.hpp that gcc had reported 1,596 times in the very CI run for the
branch that introduced its neighbours -- twice per build, across 15 projects, in a
log with 75 other warning sites for company. A count that large is indistinguishable
from zero if nobody looks.

This keeps a checked-in baseline of what is currently tolerated, fails a build that
adds to it, and rewrites it downwards on its own when a full build of main finds
fewer. Nothing here enables a new warning; it only stops the existing ones from
being free.

Warnings are keyed on (repo-relative path, warning flag) with a count, deliberately
NOT on line numbers: a fingerprint that includes the line churns on every edit above
a pre-existing warning. The cost is that swapping one -Wformat= site for another in
the same file passes; the benefit is a baseline that only moves when the warnings do.

Counts aggregate across projects with max(), not sum(): the same SDK header is
compiled by every app, so a sum would depend on how many projects a run happened to
build. max() makes the baseline a property of the code, which in turn lets a partial
build (a PR touching one app) be checked against a baseline produced by a full one.

A count is only meaningful if the flags that produce it are still on the command
line, and nothing in a build log proves that -- una/Makefile compiles with a leading
`@`, so the flags never reach the log at all. Dropping -Wcast-qual from one app makes
its warnings vanish, which reads to `check` as a fix and to `update` as a reason to
delete those keys for good. `flags` closes that: it reads the WARN/CXXWARN lists out
of every una/Makefile and fails if a required warning is gone, a new suppression
appeared, or -w turned up in user_cflags.

Subcommands:
  extract  one build log         -> normalized counts (TSV on stdout)
  check    counts vs baseline    -> exit 1 if anything is new or higher
  update   counts -> baseline    -> rewrite; 0 changed, 2 unchanged, 1 increase, 3 collapse
  compare  two baselines         -> exit 1 if the second tolerates more than the first
  flags    every una/Makefile    -> exit 1 if the warning flags were weakened
  --selftest  run the tests baked into this file, no checkout needed
"""

import argparse
import io
import os
import posixpath
import re
import sys
import tempfile
import textwrap
import unittest

# gcc: "path:line:col: warning: message [-Wflag]". The column is optional -- a few
# diagnostics (and any driver-level ones) omit it.
WARNING_RE = re.compile(r"^(?P<path>\S.*?):(?P<line>\d+)(?::(?P<col>\d+))?: warning: (?P<msg>.*)$")
FLAG_RE = re.compile(r"\[(-W[A-Za-z0-9=+-]+)\]\s*$")

# Not our code; upstream's warnings are not ours to ratchet down.
EXCLUDED_PREFIXES = ("ThirdParty/",)

# GNU make reports its own diagnostics as "Makefile:224: warning: overriding recipe",
# which is indistinguishable from gcc's shape. Counting those would let a Makefile
# edit fail the gate, and would bake a non-compiler warning into the baseline.
IGNORED_BASENAME_RE = re.compile(r"^([Mm]akefile(\..*)?|.*\.mk)$")

NO_FLAG = "(unflagged)"

# A full build that suddenly reports almost nothing is far more likely to be a broken
# parser or a stripped flag than 40 hand-fixed warnings, and `update` is the one place
# that can make the mistake permanent. Anything below this fraction of the previous
# total needs --allow-collapse. Below COLLAPSE_MIN_TOTAL there is nothing left worth
# guarding, and every honest fix would trip it.
COLLAPSE_FLOOR = 0.5
COLLAPSE_MIN_TOTAL = 10

BASELINE_HEADER = """\
# Tolerated compiler warnings in the gcc simulator build -- see
# .github/scripts/warning_baseline.py.
#
# Generated. Do not hand-edit to make a build pass: the number is the count of
# warning sites of that flag in that file, and raising one is how a warning gets
# in. It is rewritten downwards automatically when a push to main builds every
# project and finds fewer.
#
# path<TAB>flag<TAB>count
"""


def normalize_path(raw, base):
    """Return the repo-relative path, or None if it is outside the repo or excluded.

    `base` is the repo-relative directory `raw` is relative to: "" for a path the
    caller already made workspace-relative by stripping the workspace prefix, and the
    make cwd for one gcc emitted relative (`gui/src/...`, `../../Libs/Sources/...`).
    """
    path = raw.replace("\\", "/")

    if posixpath.isabs(path):
        return None  # absolute and not under the workspace: a system header

    resolved = posixpath.normpath(posixpath.join(base, path))
    if resolved == ".." or resolved.startswith("../"):
        return None  # escaped the repo root
    if resolved.startswith(EXCLUDED_PREFIXES):
        return None
    return resolved


def extract_sites(log_path, app_dir, workspace):
    """Parse one build log into a set of distinct (path, flag, line, col, msg) sites."""
    workspace = workspace.replace("\\", "/").rstrip("/")
    sites = set()

    with open(log_path, "r", encoding="utf-8", errors="replace") as fh:
        for raw_line in fh:
            match = WARNING_RE.match(raw_line.rstrip("\n"))
            if not match:
                continue

            raw_path = match.group("path")
            # A path under the workspace is repo-relative once the prefix is gone;
            # only a genuinely relative one is relative to the make cwd.
            if workspace and raw_path.startswith(workspace + "/"):
                raw_path = raw_path[len(workspace) + 1 :]
                base = ""
            else:
                base = app_dir

            path = normalize_path(raw_path, base)
            if path is None:
                continue
            if IGNORED_BASENAME_RE.match(posixpath.basename(path)):
                continue

            # A warning in a header is re-emitted once per translation unit that
            # includes it, so dedupe on the site before counting.
            msg = match.group("msg")
            flag_match = FLAG_RE.search(msg)
            flag = flag_match.group(1) if flag_match else NO_FLAG
            sites.add((path, flag, match.group("line"), match.group("col") or "", msg))

    return sites


def counts_from_sites(sites):
    counts = {}
    for path, flag, _line, _col, _msg in sites:
        counts[(path, flag)] = counts.get((path, flag), 0) + 1
    return counts


def extract(log_path, app_dir, workspace):
    """Parse one build log into {(path, flag): count} of distinct warning sites."""
    return counts_from_sites(extract_sites(log_path, app_dir, workspace))


def write_sites(fh, sites):
    for path, flag, line, col, msg in sorted(sites):
        fh.write(f"{path}\t{flag}\t{line}\t{col}\t{msg}\n")


def read_sites(paths):
    """Merge extract --details sidecars into {(path, flag): [(line, col, msg), ...]}."""
    merged = {}
    for path in paths:
        if not os.path.exists(path):
            continue
        with open(path, "r", encoding="utf-8") as fh:
            for line in fh:
                fields = line.rstrip("\n").split("\t", 4)
                if len(fields) != 5:
                    continue
                merged.setdefault((fields[0], fields[1]), set()).add(tuple(fields[2:]))
    return merged


def read_counts(path):
    """Read a TSV of path<TAB>flag<TAB>count, ignoring comments and blanks."""
    counts = {}
    if not os.path.exists(path):
        return counts
    with open(path, "r", encoding="utf-8") as fh:
        for lineno, line in enumerate(fh, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fields = line.split("\t")
            if len(fields) != 3:
                sys.exit(f"{path}:{lineno}: expected 3 tab-separated fields, got {len(fields)}")
            try:
                count = int(fields[2])
            except ValueError:
                sys.exit(f"{path}:{lineno}: count is not an integer: {fields[2]!r}")
            key = (fields[0], fields[1])
            # max(), not +=, so concatenated per-project files aggregate correctly.
            counts[key] = max(counts.get(key, 0), count)
    return counts


def write_counts(fh, counts, header=""):
    if header:
        fh.write(header)
    for (path, flag), count in sorted(counts.items()):
        fh.write(f"{path}\t{flag}\t{count}\n")


def merge(files):
    merged = {}
    for path in files:
        for key, count in read_counts(path).items():
            merged[key] = max(merged.get(key, 0), count)
    return merged


MAX_SITES_SHOWN = 6


def format_regressions(regressions, details=None):
    """One line per regressed key, plus the concrete sites when a sidecar has them.

    Without the sites a contributor knows the file and the flag but has to download
    the build.log artifact to find out which line -- which is most of the cost of
    reacting to this gate at all.
    """
    lines = []
    width = max(len(f"{p} [{f}]") for p, f in regressions) if regressions else 0
    for key, (observed, allowed) in sorted(regressions.items()):
        path, flag = key
        label = f"{path} [{flag}]"
        lines.append(f"  {label:<{width}}  {allowed} allowed -> {observed} found")
        for line, col, msg in sorted((details or {}).get(key, []), key=_site_order)[
            :MAX_SITES_SHOWN
        ]:
            where = f"{path}:{line}:{col}" if col else f"{path}:{line}"
            lines.append(f"      {where}: {msg}")
    return lines


def _site_order(site):
    line, col, _msg = site
    return (int(line) if line.isdigit() else 0, int(col) if col.isdigit() else 0)


def diff_counts(observed, allowed):
    """Keys where observed exceeds allowed, as {key: (observed, allowed)}."""
    return {
        key: (count, allowed.get(key, 0))
        for key, count in observed.items()
        if count > allowed.get(key, 0)
    }


# ------------------------------------------------------------------- flag policy

# Every una/Makefile must still ASK for these. Deleting one from the list below is
# how you legitimately retire a warning -- and it is a diff a reviewer can see, which
# editing a Makefile in one app out of fifteen is not.
REQUIRED_WARNINGS = frozenset(
    {
        "all",
        "extra",
        "format=2",
        "cast-qual",
        "write-strings",
        "init-self",
        "pointer-arith",
        "strict-aliasing",
        "uninitialized",
        "missing-declarations",
    }
)
REQUIRED_CXX_WARNINGS = frozenset({"non-virtual-dtor", "ctor-dtor-privacy"})

# The suppressions the tree already carries. A new -Wno-* silences warnings just as
# effectively as deleting the flag that finds them, so the set is closed.
ALLOWED_SUPPRESSIONS = frozenset(
    {
        "no-long-long",
        "no-unused-parameter",
        "no-variadic-macros",
        "no-format-extra-args",
        "no-conversion",
        "no-overloaded-virtual",
    }
)

# -Wno-error is the documented status quo: the Makefile asks for -Werror and then
# takes it back, which is why these are warnings and not build failures.
ALLOWED_CFLAG_SUPPRESSIONS = frozenset({"-Wno-error"})

MAKEFILE_SEARCH_ROOTS = ("Examples/Apps", "Docs/Tutorials")

# una/Makefile owns the warning lists, but it is not the only file that reaches the
# command line. config/gcc/app.mk is included before them and already sets
# user_cflags in one project; simulator/gcc/Makefile exports into the same sub-make.
# Both are scanned for suppressions -- checking only una/Makefile would leave `-w`
# one line away in a file nobody was looking at.
SUPPRESSION_SCAN_SIBLINGS = ("config/gcc/app.mk", "simulator/gcc/Makefile")

ASSIGN_RE = re.compile(r"^(?P<name>WARN|CXXWARN)\s*(?P<op>[:+?]?=)\s*(?P<value>.*)$")
ANY_ASSIGN_RE = re.compile(
    r"^(?:override\s+|export\s+)*(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*[:+?]?=\s*(?P<value>.*)$"
)
FLAG_VAR_RE = re.compile(r"cflags|cxxflags|compiler_options", re.IGNORECASE)


def _logical_lines(text):
    """Makefile lines with backslash continuations joined."""
    return re.sub(r"\\\n\s*", " ", text).splitlines()


def check_suppressions(text):
    """Problems with any make fragment that assigns to a compiler-flag variable."""
    problems = []
    for line in _logical_lines(text):
        assign = ANY_ASSIGN_RE.match(line.strip())
        if not assign or not FLAG_VAR_RE.search(assign.group("name")):
            continue
        name = assign.group("name")
        for token in assign.group("value").split():
            if token == "-w":
                problems.append(f"{name} disables all warnings with -w")
            elif token.startswith("-Wno-") and token not in ALLOWED_CFLAG_SUPPRESSIONS:
                problems.append(f"{name} adds the suppression {token}")
    return problems


def check_makefile_flags(text):
    """Return a list of human-readable problems with one una/Makefile's warning flags."""
    problems = []
    lists = {}
    uses_warn = set()

    for line in _logical_lines(text):
        stripped = line.strip()
        assign = ASSIGN_RE.match(stripped)
        if assign:
            # Follow make's own semantics for the operator. Reading every form as a
            # replacement would report the first line's flags as missing on an
            # ordinary `WARN = ...` / `WARN += ...` split, and -- worse -- would let
            # a suppression on the first line hide behind a clean += on the last.
            name, tokens = assign.group("name"), assign.group("value").split()
            if assign.group("op") == "+=":
                lists.setdefault(name, []).extend(tokens)
            elif assign.group("op") == "?=":
                lists.setdefault(name, tokens)   # make ignores ?= once the var is set
            else:
                lists[name] = tokens
            continue
        for var in ("c_compiler_options_local", "cpp_compiler_options_local"):
            if stripped.startswith(var) and "$(WARN)" in stripped:
                uses_warn.add(var)

    for name, required in (("WARN", REQUIRED_WARNINGS), ("CXXWARN", REQUIRED_CXX_WARNINGS)):
        if name not in lists:
            problems.append(f"{name} is not defined")
            continue
        tokens = set(lists[name])
        for missing in sorted(required - tokens):
            problems.append(f"{name} no longer asks for -W{missing}")
        for token in sorted(tokens):
            if token.startswith("no-") and token not in ALLOWED_SUPPRESSIONS:
                problems.append(f"{name} adds the suppression -W{token}")

    for var in ("c_compiler_options_local", "cpp_compiler_options_local"):
        if var not in uses_warn:
            problems.append(f"{var} no longer expands $(WARN)")
    if "-pedantic" not in text:
        problems.append("-pedantic is gone")

    return problems + check_suppressions(text)


def find_una_makefiles(root):
    """Every <project>/una/Makefile under the app and tutorial trees, repo-relative."""
    found = []
    for base in MAKEFILE_SEARCH_ROOTS:
        for dirpath, _dirnames, filenames in os.walk(os.path.join(root, base)):
            if os.path.basename(dirpath) == "una" and "Makefile" in filenames:
                full = os.path.join(dirpath, "Makefile")
                found.append(os.path.relpath(full, root).replace(os.sep, "/"))
    return sorted(found)


def cmd_flags(args):
    root = args.root or REPO_ROOT
    makefiles = find_una_makefiles(root)

    # Finding none would otherwise pass silently and vouch for nothing.
    if not makefiles:
        print(f"FAIL: no una/Makefile found under {'/, '.join(MAKEFILE_SEARCH_ROOTS)}/ in {root}")
        return 1

    def report(rel, problems):
        if not problems:
            return 0
        print(f"FAIL {rel}")
        for problem in problems:
            print(f"  - {problem}")
        return 1

    failures = 0
    scanned = 0
    for rel in makefiles:
        with open(os.path.join(root, rel), "r", encoding="utf-8", errors="replace") as fh:
            failures += report(rel, check_makefile_flags(fh.read()))
        scanned += 1

        project = posixpath.dirname(posixpath.dirname(rel))
        for sibling in SUPPRESSION_SCAN_SIBLINGS:
            sib_rel = posixpath.join(project, sibling)
            sib_abs = os.path.join(root, sib_rel)
            if not os.path.exists(sib_abs):
                continue
            with open(sib_abs, "r", encoding="utf-8", errors="replace") as fh:
                failures += report(sib_rel, check_suppressions(fh.read()))
            scanned += 1

    if failures:
        print(
            f"\n{failures} file(s) across {len(makefiles)} simulator project(s) weakened the "
            "warning flags.\nThe baseline counts warnings the build reports; a flag that is no "
            "longer requested\nreports nothing, which reads as a fix and is then ratcheted away "
            "for good. Retire a\nwarning by removing it from REQUIRED_WARNINGS in this script -- "
            "in one reviewable\nplace -- not from one app's Makefile."
        )
        return 1

    print(
        f"OK: {len(makefiles)} simulator project(s) still request the required warning flags "
        f"({scanned} make fragment(s) scanned)."
    )
    return 0


def cmd_extract(args):
    sites = extract_sites(args.log, args.app_dir.strip("/"), args.workspace)
    counts = counts_from_sites(sites)
    out = open(args.out, "w", encoding="utf-8") if args.out else sys.stdout
    try:
        write_counts(out, counts)
    finally:
        if args.out:
            out.close()
    if args.details:
        with open(args.details, "w", encoding="utf-8") as fh:
            write_sites(fh, sites)
    total = sum(counts.values())
    print(f"{args.log}: {total} warning site(s) in {len(counts)} file/flag pair(s)", file=sys.stderr)
    return 0


def cmd_check(args):
    baseline = read_counts(args.baseline)
    observed = merge(args.counts)
    regressions = diff_counts(observed, baseline)

    if not regressions:
        print(
            f"OK: {sum(observed.values())} warning site(s), none beyond the "
            f"{sum(baseline.values())}-site baseline."
        )
        return 0

    new_sites = sum(o - a for o, a in regressions.values())
    print(f"FAIL: {new_sites} new warning site(s) in {len(regressions)} file/flag pair(s):\n")
    print("\n".join(format_regressions(regressions, read_sites(args.details))))
    print(
        "\nFix the warnings. If a warning is genuinely acceptable, raising its count in\n"
        f"{args.baseline} is a reviewable change -- not a silent one."
    )
    return 1


def cmd_update(args):
    old = read_counts(args.baseline)
    new = merge(args.counts)

    # Never ratchet up here: this runs on a full build of main, where an increase
    # means a warning slipped past the gate and should stay visible as a failure
    # rather than being absorbed into the baseline.
    regressions = diff_counts(new, old)
    if regressions and not args.allow_increase:
        print(f"REFUSING to update: {len(regressions)} file/flag pair(s) increased:\n")
        print("\n".join(format_regressions(regressions)))
        return 1

    # A collapse is the one direction the ratchet cannot walk back: once the keys are
    # gone, restoring the flags that found them fails `check` on every PR, so the gate
    # ends up blocking its own repair. Make a big drop an explicit decision.
    old_total, new_total = sum(old.values()), sum(new.values())
    if (
        old_total >= COLLAPSE_MIN_TOTAL
        and new_total < old_total * COLLAPSE_FLOOR
        and not args.allow_collapse
    ):
        print(
            f"REFUSING to update: {old_total} -> {new_total} warning site(s) is a "
            f"{100 * (old_total - new_total) // old_total}% drop.\n\n"
            "A drop that size is more often a build that stopped asking for the flags, or\n"
            "a parser that stopped recognising the output, than a real fix. Confirm the\n"
            "warnings are genuinely gone, then re-run with --allow-collapse."
        )
        return 3

    if new == old:
        print(f"Baseline unchanged ({sum(old.values())} warning site(s)).")
        return 2

    with open(args.baseline, "w", encoding="utf-8") as fh:
        write_counts(fh, new, BASELINE_HEADER)
    removed = sum(old.values()) - sum(new.values())
    verb = f"Ratcheted down ({removed} fewer)" if removed > 0 else "Rewrote baseline"
    print(
        f"{verb}: {sum(old.values())} -> {sum(new.values())} warning site(s), "
        f"{len(old)} -> {len(new)} file/flag pair(s)."
    )
    return 0


def cmd_compare(args):
    """Flag a baseline that got more permissive -- i.e. hand-raised to pass the gate."""
    before = read_counts(args.before)
    after = read_counts(args.after)
    raised = diff_counts(after, before)

    if not raised:
        print("Baseline is not more permissive than the base revision's.")
        return 0

    print(f"Baseline RAISED for {len(raised)} file/flag pair(s):\n")
    print("\n".join(format_regressions(raised)))
    return 1


REPO_ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

# --------------------------------------------------------------------------- tests

SELFTEST_WORKSPACE = "/home/runner/work/una-sdk/una-sdk"
SELFTEST_APP_DIR = "Examples/Apps/Hiking/Software/Apps/TouchGFX-GUI"

# One warning per path shape the real build emits, plus the noise the parser has to
# ignore. Trimmed from an actual linux-simulator.yml build.log.
SELFTEST_LOG = textwrap.dedent(
    f"""\
    g++ -c -Wall -Wextra -o build/foo.o foo.cpp
    In file included from {SELFTEST_WORKSPACE}/Libs/Header/SDK/Simulator/Kernel/Mock/AppMemory.hpp:12:
    {SELFTEST_WORKSPACE}/Libs/Header/SDK/Simulator/Kernel/Mock/AppMemory.hpp:30:28: warning: format '%d' expects argument of type 'int', but argument 3 has type 'size_t' [-Wformat=]
    {SELFTEST_WORKSPACE}/Libs/Header/SDK/Simulator/Kernel/Mock/AppMemory.hpp:42:30: warning: format '%d' expects argument of type 'int', but argument 3 has type 'size_t' [-Wformat=]
    {SELFTEST_WORKSPACE}/Libs/Header/SDK/Simulator/OS/OS.hpp:71:9: warning: 'OS::Mutex::mHandle' will be initialized after [-Wreorder]
    gui/src/main_screen/MainView.cpp:208:73: warning: format '%lu' expects argument of type 'long unsigned int' [-Wformat=]
    ../../Libs/Sources/Service.cpp:286:16: warning: ISO C++ prohibits anonymous structs [-Wpedantic]
    ../../../../../../ThirdParty/touchgfx/framework/source/platform/hal/simulator/sdl2/HALSDL2.cpp:1335:38: warning: '%02d' directive output may be truncated [-Wformat-truncation=]
    {SELFTEST_WORKSPACE}/ThirdParty/coreJSON/source/core_json.c:88:5: warning: unused variable 'x' [-Wunused-variable]
    /usr/include/c++/13/bits/stl_algo.h:120:5: warning: some libstdc++ noise [-Wunused-variable]
    ::warning::a GitHub Actions annotation, not a compiler warning
    make: *** [Makefile:224: build/foo.o] Error 1
    """
)


class SelftestBase(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)

    def write(self, name, text):
        path = os.path.join(self.tmp.name, name)
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(text)
        return path


class TestExtract(SelftestBase):
    def setUp(self):
        super().setUp()
        self.log = self.write("build.log", SELFTEST_LOG)
        self.counts = extract(self.log, SELFTEST_APP_DIR, SELFTEST_WORKSPACE)

    def test_workspace_paths_become_repo_relative(self):
        self.assertEqual(
            self.counts[("Libs/Header/SDK/Simulator/Kernel/Mock/AppMemory.hpp", "-Wformat=")], 2
        )
        self.assertEqual(self.counts[("Libs/Header/SDK/Simulator/OS/OS.hpp", "-Wreorder")], 1)

    def test_cwd_relative_paths_resolve_against_the_app_dir(self):
        self.assertEqual(
            self.counts[(f"{SELFTEST_APP_DIR}/gui/src/main_screen/MainView.cpp", "-Wformat=")], 1
        )
        self.assertEqual(
            self.counts[("Examples/Apps/Hiking/Software/Libs/Sources/Service.cpp", "-Wpedantic")], 1
        )

    def test_thirdparty_is_excluded_by_either_path_shape(self):
        for path, _flag in self.counts:
            self.assertNotIn("ThirdParty", path)

    def test_paths_outside_the_repo_are_dropped(self):
        for path, _flag in self.counts:
            self.assertFalse(path.startswith("/"), path)
            self.assertFalse(path.startswith(".."), path)

    def test_non_compiler_lines_are_ignored(self):
        # 5 first-party sites; the annotation, the "In file included from" line, the
        # compile command and the make error must not register.
        self.assertEqual(sum(self.counts.values()), 5)

    def test_a_header_warning_counts_once_per_site_not_per_tu(self):
        # The same sites, re-emitted for a second translation unit.
        doubled = self.write("doubled.log", SELFTEST_LOG + SELFTEST_LOG)
        self.assertEqual(extract(doubled, SELFTEST_APP_DIR, SELFTEST_WORKSPACE), self.counts)

    def test_a_warning_without_a_column_still_parses(self):
        log = self.write(
            "nocol.log",
            f"{SELFTEST_WORKSPACE}/Libs/Source/Simulator/OS/OS.cpp:12: warning: x [-Wpedantic]\n",
        )
        self.assertEqual(
            extract(log, SELFTEST_APP_DIR, SELFTEST_WORKSPACE),
            {("Libs/Source/Simulator/OS/OS.cpp", "-Wpedantic"): 1},
        )

    def test_a_warning_without_a_flag_lands_in_the_unflagged_bucket(self):
        log = self.write(
            "noflag.log",
            f"{SELFTEST_WORKSPACE}/Libs/Source/Simulator/OS/OS.cpp:12:1: warning: no flag\n",
        )
        self.assertEqual(
            extract(log, SELFTEST_APP_DIR, SELFTEST_WORKSPACE),
            {("Libs/Source/Simulator/OS/OS.cpp", NO_FLAG): 1},
        )

    def test_line_numbers_are_not_part_of_the_key(self):
        """Editing above a pre-existing warning must not churn the baseline."""
        moved = self.write("moved.log", SELFTEST_LOG.replace(":30:28:", ":130:28:"))
        self.assertEqual(extract(moved, SELFTEST_APP_DIR, SELFTEST_WORKSPACE), self.counts)

    def test_make_diagnostics_are_not_counted_as_code_warnings(self):
        """`Makefile:224: warning: overriding recipe` has gcc's exact shape."""
        log = self.write(
            "make.log",
            "Makefile:224: warning: overriding recipe for target 'x'\n"
            "una/Makefile:12: warning: ignoring old recipe\n"
            "config/gcc/app.mk:3:1: warning: something [-Wpedantic]\n",
        )
        self.assertEqual(extract(log, SELFTEST_APP_DIR, SELFTEST_WORKSPACE), {})

    def test_details_carry_the_line_and_message(self):
        sites = extract_sites(self.log, SELFTEST_APP_DIR, SELFTEST_WORKSPACE)
        key = ("Libs/Header/SDK/Simulator/Kernel/Mock/AppMemory.hpp", "-Wformat=")
        lines = {line for path, flag, line, _col, _msg in sites if (path, flag) == key}
        self.assertEqual(lines, {"30", "42"})


class TestBaselineIO(SelftestBase):
    def test_round_trip_through_the_file_format(self):
        counts = {("Libs/a.cpp", "-Wformat="): 2, ("Libs/b.hpp", "-Wreorder"): 1}
        path = os.path.join(self.tmp.name, "baseline.txt")
        with open(path, "w", encoding="utf-8") as fh:
            write_counts(fh, counts, BASELINE_HEADER)
        self.assertEqual(read_counts(path), counts)

    def test_a_missing_baseline_reads_as_empty(self):
        self.assertEqual(read_counts(os.path.join(self.tmp.name, "nope.txt")), {})

    def test_merge_takes_the_max_across_projects_not_the_sum(self):
        """The same SDK header is compiled by every app; summing would make the
        baseline depend on how many projects a run happened to build."""
        a = self.write("a.tsv", "Libs/x.hpp\t-Wreorder\t2\n")
        b = self.write("b.tsv", "Libs/x.hpp\t-Wreorder\t2\n")
        self.assertEqual(merge([a, b]), {("Libs/x.hpp", "-Wreorder"): 2})

    def test_merge_keeps_the_higher_count_when_projects_differ(self):
        a = self.write("a.tsv", "Libs/x.hpp\t-Wreorder\t1\n")
        b = self.write("b.tsv", "Libs/x.hpp\t-Wreorder\t3\n")
        self.assertEqual(merge([a, b]), {("Libs/x.hpp", "-Wreorder"): 3})

    def test_a_malformed_line_is_a_hard_error(self):
        bad = self.write("bad.tsv", "Libs/x.hpp\t-Wreorder\tnot-a-number\n")
        with self.assertRaises(SystemExit):
            read_counts(bad)


class TestCommands(SelftestBase):
    def setUp(self):
        super().setUp()
        self.baseline = self.write(
            "baseline.txt", "Libs/a.cpp\t-Wformat=\t2\nLibs/b.hpp\t-Wreorder\t1\n"
        )

    def test_check_passes_when_observed_matches(self):
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t2\nLibs/b.hpp\t-Wreorder\t1\n")
        self.assertEqual(main(["check", "--baseline", self.baseline, obs]), 0)

    def test_check_passes_on_a_partial_build(self):
        """A PR touching one app builds only that app, so it legitimately sees a
        subset of the baseline. Fewer warnings is never a failure."""
        obs = self.write("o.tsv", "Libs/b.hpp\t-Wreorder\t1\n")
        self.assertEqual(main(["check", "--baseline", self.baseline, obs]), 0)

    def test_check_fails_on_a_brand_new_warning(self):
        obs = self.write("o.tsv", "Libs/c.cpp\t-Wformat=\t1\n")
        self.assertEqual(main(["check", "--baseline", self.baseline, obs]), 1)

    def test_check_fails_on_one_more_of_an_existing_warning(self):
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t3\n")
        self.assertEqual(main(["check", "--baseline", self.baseline, obs]), 1)

    def test_check_fails_on_a_new_flag_in_an_already_warning_file(self):
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wswitch\t1\n")
        self.assertEqual(main(["check", "--baseline", self.baseline, obs]), 1)

    def test_update_ratchets_down_and_rewrites(self):
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t1\nLibs/b.hpp\t-Wreorder\t1\n")
        self.assertEqual(main(["update", "--baseline", self.baseline, obs]), 0)
        self.assertEqual(
            read_counts(self.baseline),
            {("Libs/a.cpp", "-Wformat="): 1, ("Libs/b.hpp", "-Wreorder"): 1},
        )

    def test_update_drops_a_key_that_went_to_zero(self):
        obs = self.write("o.tsv", "Libs/b.hpp\t-Wreorder\t1\n")
        self.assertEqual(main(["update", "--baseline", self.baseline, obs]), 0)
        self.assertEqual(read_counts(self.baseline), {("Libs/b.hpp", "-Wreorder"): 1})

    def test_update_is_idempotent(self):
        """Exit 2 means nothing changed, so the workflow skips an empty commit."""
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t2\nLibs/b.hpp\t-Wreorder\t1\n")
        self.assertEqual(main(["update", "--baseline", self.baseline, obs]), 2)

    def test_update_refuses_to_absorb_an_increase(self):
        """On main an increase means a warning slipped past the gate; it has to stay
        visible as a failure rather than becoming the new normal."""
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t9\n")
        self.assertEqual(main(["update", "--baseline", self.baseline, obs]), 1)
        self.assertEqual(read_counts(self.baseline)[("Libs/a.cpp", "-Wformat=")], 2)

    def test_update_allows_an_increase_only_when_asked(self):
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t9\n")
        self.assertEqual(main(["update", "--baseline", self.baseline, "--allow-increase", obs]), 0)

    def test_update_refuses_a_collapse(self):
        """A near-empty observation is what a stripped flag or a broken parser looks
        like, and deleting the keys is the one direction the ratchet cannot walk
        back: restoring the flags afterwards fails `check` on every PR."""
        big = self.write("big.txt", "Libs/a.cpp\t-Wformat=\t20\n")
        obs = self.write("o.tsv", "")
        self.assertEqual(main(["update", "--baseline", big, obs]), 3)
        self.assertEqual(sum(read_counts(big).values()), 20)

    def test_update_allows_a_collapse_only_when_asked(self):
        big = self.write("big.txt", "Libs/a.cpp\t-Wformat=\t20\n")
        obs = self.write("o.tsv", "")
        self.assertEqual(main(["update", "--baseline", big, "--allow-collapse", obs]), 0)
        self.assertEqual(read_counts(big), {})

    def test_update_allows_a_drop_that_stays_above_the_floor(self):
        big = self.write("big.txt", "Libs/a.cpp\t-Wformat=\t20\n")
        obs = self.write("o.tsv", "Libs/a.cpp\t-Wformat=\t15\n")
        self.assertEqual(main(["update", "--baseline", big, obs]), 0)

    def test_a_tiny_baseline_is_not_subject_to_the_collapse_floor(self):
        """Fixing the last two warnings must not need a flag."""
        obs = self.write("o.tsv", "")
        self.assertEqual(main(["update", "--baseline", self.baseline, obs]), 0)

    def test_check_names_the_offending_lines_when_details_are_supplied(self):
        obs = self.write("o.tsv", "Libs/c.cpp\t-Wformat=\t1\n")
        det = self.write("o.details.tsv", "Libs/c.cpp\t-Wformat=\t7\t3\tformat '%d' expects int\n")
        out = io.StringIO()
        real, sys.stdout = sys.stdout, out
        try:
            self.assertEqual(
                main(["check", "--baseline", self.baseline, "--details", det, obs]), 1
            )
        finally:
            sys.stdout = real
        self.assertIn("Libs/c.cpp:7:3: format '%d' expects int", out.getvalue())

    def test_compare_flags_a_hand_raised_baseline(self):
        after = self.write("after.txt", "Libs/a.cpp\t-Wformat=\t5\n")
        self.assertEqual(main(["compare", "--before", self.baseline, "--after", after]), 1)

    def test_compare_accepts_a_lowered_baseline(self):
        after = self.write("after.txt", "Libs/a.cpp\t-Wformat=\t1\n")
        self.assertEqual(main(["compare", "--before", self.baseline, "--after", after]), 0)

    def test_extract_writes_the_requested_file(self):
        log = self.write("build.log", SELFTEST_LOG)
        out = os.path.join(self.tmp.name, "counts.tsv")
        argv = ["extract", "--log", log, "--app-dir", SELFTEST_APP_DIR,
                "--workspace", SELFTEST_WORKSPACE, "--out", out]
        self.assertEqual(main(argv), 0)
        self.assertEqual(
            read_counts(out)[("Libs/Header/SDK/Simulator/Kernel/Mock/AppMemory.hpp", "-Wformat=")], 2
        )


GOOD_MAKEFILE = textwrap.dedent(
    """\
    WARN = error all extra write-strings init-self cast-qual \\
           pointer-arith strict-aliasing format=2 uninitialized \\
           missing-declarations no-long-long no-unused-parameter \\
           no-variadic-macros no-format-extra-args \\
           no-conversion no-overloaded-virtual
    CXXWARN = non-virtual-dtor ctor-dtor-privacy

    c_compiler_options_local   += -pedantic $(addprefix -W,$(WARN))
    cpp_compiler_options_local += -pedantic $(addprefix -W,$(WARN) $(CXXWARN))
    override user_cflags += -Wno-error
    override user_cflags += -DBUILD_VERSION=\\"1.2.3\\"
    """
)


class TestFlagPolicy(unittest.TestCase):
    """A count only means something while the flags that produce it are requested."""

    def test_the_tree_as_it_stands_is_accepted(self):
        self.assertEqual(check_makefile_flags(GOOD_MAKEFILE), [])

    def test_a_dropped_warning_is_caught(self):
        weakened = GOOD_MAKEFILE.replace(" cast-qual", "")
        self.assertIn("WARN no longer asks for -Wcast-qual", check_makefile_flags(weakened))

    def test_a_new_suppression_is_caught(self):
        weakened = GOOD_MAKEFILE.replace("no-long-long", "no-long-long no-cast-qual")
        self.assertIn("WARN adds the suppression -Wno-cast-qual", check_makefile_flags(weakened))

    def test_a_dropped_cxx_warning_is_caught(self):
        weakened = GOOD_MAKEFILE.replace("non-virtual-dtor ", "")
        self.assertIn("CXXWARN no longer asks for -Wnon-virtual-dtor", check_makefile_flags(weakened))

    def test_blanket_w_in_user_cflags_is_caught(self):
        weakened = GOOD_MAKEFILE.replace("-Wno-error", "-Wno-error -w")
        self.assertIn("user_cflags disables all warnings with -w", check_makefile_flags(weakened))

    def test_a_suppression_smuggled_into_user_cflags_is_caught(self):
        weakened = GOOD_MAKEFILE.replace("-Wno-error", "-Wno-error -Wno-reorder")
        self.assertIn(
            "user_cflags adds the suppression -Wno-reorder", check_makefile_flags(weakened)
        )

    def test_detaching_the_list_from_the_command_line_is_caught(self):
        """Leaving WARN defined but unused would satisfy a naive grep."""
        weakened = GOOD_MAKEFILE.replace("$(addprefix -W,$(WARN))", "")
        self.assertIn(
            "c_compiler_options_local no longer expands $(WARN)", check_makefile_flags(weakened)
        )

    def test_dropping_pedantic_is_caught(self):
        self.assertIn("-pedantic is gone", check_makefile_flags(GOOD_MAKEFILE.replace("-pedantic", "")))

    def test_flags_split_across_an_append_are_accumulated(self):
        """`WARN = ...` then `WARN += ...` is ordinary make. Reading only the last
        assignment reported everything on the first line as missing."""
        split = GOOD_MAKEFILE.replace(
            "WARN = error all extra write-strings",
            "WARN = error all extra\nWARN += write-strings",
        )
        self.assertEqual(check_makefile_flags(split), [])

    def test_a_suppression_hidden_behind_a_later_append_is_caught(self):
        """The bypass the accumulation fixes: put -Wno-pedantic in the first
        assignment and every required flag in a clean += on the last. make puts the
        suppression on the command line ahead of the flags, where nothing re-enables
        it, and reading only the last assignment saw nothing wrong."""
        hidden = GOOD_MAKEFILE.replace("WARN = error", "WARN = no-pedantic\nWARN += error")
        self.assertIn("WARN adds the suppression -Wno-pedantic", check_makefile_flags(hidden))

    def test_a_conditional_assignment_does_not_override_an_earlier_one(self):
        """make ignores ?= once the variable is set, so the check must too."""
        conditional = GOOD_MAKEFILE + "WARN ?= all\n"
        self.assertEqual(check_makefile_flags(conditional), [])

    def test_the_real_app_mk_is_accepted(self):
        """config/gcc/app.mk legitimately sets user_cflags in one project."""
        self.assertEqual(
            check_suppressions("touchgfx_path := ../../ThirdParty/touchgfx\nuser_cflags := -DUSE_BPP=8\n"),
            [],
        )

    def test_a_sibling_fragment_smuggling_w_is_caught(self):
        """app.mk is included before una/Makefile's own flags and survives the += it
        does; scanning only una/Makefile would leave -w one line away."""
        self.assertIn(
            "user_cflags disables all warnings with -w",
            check_suppressions("user_cflags := -DUSE_BPP=8 -w\n"),
        )

    def test_a_sibling_fragment_smuggling_a_suppression_is_caught(self):
        self.assertIn(
            "CXXFLAGS adds the suppression -Wno-cast-qual",
            check_suppressions("export CXXFLAGS += -Wno-cast-qual\n"),
        )

    def test_the_addprefix_line_is_not_mistaken_for_a_suppression(self):
        self.assertEqual(
            check_suppressions("cpp_compiler_options_local += -pedantic $(addprefix -W,$(WARN))\n"),
            [],
        )


class TestCommittedMakefiles(unittest.TestCase):
    """Reads the real tree. Weakening the gate now means editing this file, in a diff
    that says so, rather than one app's Makefile out of fifteen."""

    def setUp(self):
        self.makefiles = find_una_makefiles(REPO_ROOT)
        if not self.makefiles:
            self.skipTest("no una/Makefile in this checkout")

    def test_the_headline_flags_are_required_by_name(self):
        for flag in ("all", "extra", "format=2", "cast-qual"):
            self.assertIn(flag, REQUIRED_WARNINGS, f"-W{flag} was dropped from the policy")

    def test_every_project_still_requests_them(self):
        offenders = {}
        for rel in self.makefiles:
            with open(os.path.join(REPO_ROOT, rel), "r", encoding="utf-8") as fh:
                problems = check_makefile_flags(fh.read())
            if problems:
                offenders[rel] = problems
        self.assertEqual(offenders, {})


class TestCommittedBaseline(unittest.TestCase):
    """Catches a baseline that has drifted out of sync with the tree. Skipped when
    run outside a checkout."""

    def setUp(self):
        self.baseline = os.path.join(REPO_ROOT, ".github", "warning-baseline.txt")
        if not os.path.exists(self.baseline):
            self.skipTest("no committed baseline")

    def test_it_parses_and_is_not_empty(self):
        self.assertGreater(len(read_counts(self.baseline)), 0)

    def test_every_path_still_exists(self):
        missing = [
            path
            for path, _flag in read_counts(self.baseline)
            if not os.path.exists(os.path.join(REPO_ROOT, path))
        ]
        self.assertEqual(
            missing,
            [],
            "the baseline names files that no longer exist -- a full build of main will "
            "ratchet them away, or drop the lines by hand",
        )

    def test_no_thirdparty_entries(self):
        offenders = [p for p, _f in read_counts(self.baseline) if p.startswith("ThirdParty/")]
        self.assertEqual(offenders, [])


def run_selftest():
    """--selftest runs the tests baked into this file, so a parsing regression is
    catchable without an SDK checkout or a simulator build."""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite(
        loader.loadTestsFromTestCase(case)
        for case in (
            TestExtract,
            TestBaselineIO,
            TestCommands,
            TestFlagPolicy,
            TestCommittedMakefiles,
            TestCommittedBaseline,
        )
    )
    # The command tests print their own diagnostics; keep that off the test report.
    with open(os.devnull, "w", encoding="utf-8") as devnull:
        real_stdout, sys.stdout = sys.stdout, devnull
        try:
            result = unittest.TextTestRunner(stream=real_stdout, verbosity=2).run(suite)
        finally:
            sys.stdout = real_stdout
    return 0 if result.wasSuccessful() else 1


# ---------------------------------------------------------------------------- cli


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if "--selftest" in argv:
        return run_selftest()

    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)

    p = sub.add_parser("extract", help="parse a build log into normalized warning counts")
    p.add_argument("--log", required=True, help="build log to parse")
    p.add_argument(
        "--app-dir",
        required=True,
        help="repo-relative make cwd, e.g. Examples/Apps/Hiking/Software/Apps/TouchGFX-GUI",
    )
    p.add_argument("--workspace", default="", help="absolute repo root to strip from paths")
    p.add_argument("--out", help="write here instead of stdout")
    p.add_argument("--details", help="also write path/flag/line/col/message sites here")
    p.set_defaults(func=cmd_extract)

    p = sub.add_parser("check", help="fail if counts exceed the baseline")
    p.add_argument("--baseline", required=True)
    # Repeatable rather than nargs="*", which would swallow the positional counts.
    p.add_argument(
        "--details",
        action="append",
        default=[],
        metavar="FILE",
        help="an extract --details sidecar, to name the offending lines in the failure",
    )
    p.add_argument("counts", nargs="+", help="one or more extract outputs")
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("update", help="rewrite the baseline from a full build's counts")
    p.add_argument("--baseline", required=True)
    p.add_argument(
        "--allow-increase",
        action="store_true",
        help="permit a higher baseline (for the initial seeding only)",
    )
    p.add_argument(
        "--allow-collapse",
        action="store_true",
        help=f"permit a drop below {int(COLLAPSE_FLOOR * 100)}%% of the current total",
    )
    p.add_argument("counts", nargs="+", help="one or more extract outputs")
    p.set_defaults(func=cmd_update)

    p = sub.add_parser("compare", help="fail if the second baseline tolerates more")
    p.add_argument("--before", required=True)
    p.add_argument("--after", required=True)
    p.set_defaults(func=cmd_compare)

    p = sub.add_parser("flags", help="fail if any una/Makefile weakened its warning flags")
    p.add_argument("--root", help="repo root to scan (default: this script's checkout)")
    p.set_defaults(func=cmd_flags)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
