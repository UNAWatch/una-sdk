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

Subcommands:
  extract  one build log         -> normalized counts (TSV on stdout)
  check    counts vs baseline    -> exit 1 if anything is new or higher
  update   counts -> baseline    -> rewrite, exit 0 if it changed, 2 if unchanged
  compare  two baselines         -> exit 1 if the second tolerates more than the first
  --selftest  run the tests baked into this file, no checkout needed
"""

import argparse
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

NO_FLAG = "(unflagged)"

BASELINE_HEADER = """\
# Tolerated compiler warnings in the gcc simulator build -- see
# Utilities/Scripts/warning-baseline/warning_baseline.py.
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


def extract(log_path, app_dir, workspace):
    """Parse one build log into {(path, flag): count} of distinct warning sites."""
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

            # A warning in a header is re-emitted once per translation unit that
            # includes it, so dedupe on the site before counting.
            sites.add((path, match.group("line"), match.group("col"), match.group("msg")))

    counts = {}
    for path, _line, _col, msg in sites:
        flag_match = FLAG_RE.search(msg)
        flag = flag_match.group(1) if flag_match else NO_FLAG
        counts[(path, flag)] = counts.get((path, flag), 0) + 1
    return counts


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


def format_regressions(regressions):
    lines = []
    width = max(len(f"{p} [{f}]") for p, f in regressions) if regressions else 0
    for (path, flag), (observed, allowed) in sorted(regressions.items()):
        label = f"{path} [{flag}]"
        lines.append(f"  {label:<{width}}  {allowed} allowed -> {observed} found")
    return lines


def diff_counts(observed, allowed):
    """Keys where observed exceeds allowed, as {key: (observed, allowed)}."""
    return {
        key: (count, allowed.get(key, 0))
        for key, count in observed.items()
        if count > allowed.get(key, 0)
    }


def cmd_extract(args):
    counts = extract(args.log, args.app_dir.strip("/"), args.workspace)
    out = open(args.out, "w", encoding="utf-8") if args.out else sys.stdout
    try:
        write_counts(out, counts)
    finally:
        if args.out:
            out.close()
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
    print("\n".join(format_regressions(regressions)))
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
        for case in (TestExtract, TestBaselineIO, TestCommands, TestCommittedBaseline)
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
    p.set_defaults(func=cmd_extract)

    p = sub.add_parser("check", help="fail if counts exceed the baseline")
    p.add_argument("--baseline", required=True)
    p.add_argument("counts", nargs="+", help="one or more extract outputs")
    p.set_defaults(func=cmd_check)

    p = sub.add_parser("update", help="rewrite the baseline from a full build's counts")
    p.add_argument("--baseline", required=True)
    p.add_argument(
        "--allow-increase",
        action="store_true",
        help="permit a higher baseline (for the initial seeding only)",
    )
    p.add_argument("counts", nargs="+", help="one or more extract outputs")
    p.set_defaults(func=cmd_update)

    p = sub.add_parser("compare", help="fail if the second baseline tolerates more")
    p.add_argument("--before", required=True)
    p.add_argument("--after", required=True)
    p.set_defaults(func=cmd_compare)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
