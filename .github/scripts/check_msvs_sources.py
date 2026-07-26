#!/usr/bin/env python3
"""Every example app and tutorial keeps two source lists in sync by hand: the
gcc Makefile's ADDITIONAL_SOURCES(_UNA) and the MSVS Application.vcxproj's
ClCompile items. Nobody's watching that second one, since CI only ever builds
the gcc side (Linux simulator, STM32CubeIDE), so when a file gets added to one
list and not the other, nothing complains until someone opens Visual Studio
and hits a link error.

This isn't hypothetical: #180 and #203 each fixed a missing-source link error
by patching the gcc side and left the vcxproj still broken; #207 fixed the
vcxproj but only for the two files it named, missing others that had been
drifting since #133; #76 and #53 show the two lists were out of sync from
the start.

This script just diffs the two lists per project. No MSVC, no Windows needed;
it runs anywhere Python 3 does.
"""
import os
import re
import sys
import tempfile

REPO_ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SEARCH_ROOTS = ["Examples/Apps", "Docs/Tutorials"]

# vcxproj gets to the SDK root via $(SdkPath); the Makefile gets there with a
# pile of "../" instead. Same place, different spelling, so treat them as equal.
SDK_ROOT_MACROS = ("$(SdkPath)",)
# Framework sources never show up in ADDITIONAL_SOURCES; the Makefile finds
# them on its own via framework_source, so they're not ours to compare.
FRAMEWORK_MACROS = ("$(TouchGFXReleasePath)",)
# Same deal for anything under generated/ or gui/, plus the simulator entry
# point: the Makefile's source_paths scan already picks these up on its own.
AUTO_DISCOVERED_PREFIXES = ("generated/", "gui/")
AUTO_DISCOVERED_EXACT = ("simulator/main.cpp",)


def find_projects():
    for root in SEARCH_ROOTS:
        base = os.path.join(REPO_ROOT, root)
        for dirpath, _dirnames, filenames in os.walk(base):
            if "Makefile" not in filenames:
                continue
            if os.path.basename(dirpath) != "gcc" or os.path.basename(os.path.dirname(dirpath)) != "simulator":
                continue
            makefile_path = os.path.join(dirpath, "Makefile")
            simulator_dir = os.path.dirname(dirpath)
            vcxproj_path = os.path.join(simulator_dir, "msvs", "Application.vcxproj")
            if os.path.isfile(vcxproj_path):
                yield makefile_path, vcxproj_path


def strip_leading_dotdots(path):
    while path.startswith("../"):
        path = path[3:]
    return path


def parse_makefile_sources(makefile_path):
    with open(makefile_path, "r") as f:
        lines = f.readlines()

    sources = []
    i = 0
    var_re = re.compile(r"^(ADDITIONAL_SOURCES(?:_UNA)?)\s*:=\s*(.*)$")
    while i < len(lines):
        m = var_re.match(lines[i].rstrip("\n"))
        if not m:
            i += 1
            continue
        value = m.group(2)
        entries = []
        while value.rstrip().endswith("\\"):
            entries.append(value.rstrip()[:-1].strip())
            i += 1
            value = lines[i].rstrip("\n") if i < len(lines) else ""
        entries.append(value.strip())
        i += 1
        for entry in entries:
            entry = entry.strip()
            # Heads up: if the last source line keeps its trailing "\", Make
            # just glues the next line onto this value, doesn't care that it
            # looks like a new variable. We've actually seen
            # "ADDITIONAL_SOURCES :=" show up as a "source" because of this.
            # Filtering to .cpp/.c files keeps that junk out.
            if entry and re.search(r"\.(cpp|c)$", entry):
                sources.append(strip_leading_dotdots(entry.replace("\\", "/")))
    return set(sources)


CLCOMPILE_RE = re.compile(r'<ClCompile\s+Include="([^"]+)"\s*/?>')


def parse_vcxproj_sources(vcxproj_path):
    with open(vcxproj_path, "r") as f:
        text = f.read()

    tracked = set()
    for raw in CLCOMPILE_RE.findall(text):
        path = raw.replace("\\", "/")

        if any(path.startswith(macro + "/") for macro in FRAMEWORK_MACROS):
            continue

        for macro in SDK_ROOT_MACROS:
            prefix = macro + "/"
            if path.startswith(prefix):
                path = path[len(prefix):]
                break
        else:
            # Didn't match $(SdkPath) above? Try $(ApplicationRoot) too,
            # since once we strip the dotdots, both roads land on the same path.
            for macro in ("$(ApplicationRoot)",):
                app_prefix = macro + "/"
                if path.startswith(app_prefix):
                    path = path[len(app_prefix):]
                    break

        path = strip_leading_dotdots(path)

        if path in AUTO_DISCOVERED_EXACT or path.startswith(AUTO_DISCOVERED_PREFIXES):
            continue

        tracked.add(path)
    return tracked


def project_label(makefile_path):
    rel = os.path.relpath(makefile_path, REPO_ROOT)
    return rel[: -len("/simulator/gcc/Makefile")]


def run_selftest():
    """--selftest runs these checks against fixtures baked right into this
    file, so you can catch a parsing regression without an SDK checkout."""
    failures = []

    def check(name, actual, expected):
        if actual != expected:
            failures.append(f"{name}: expected {sorted(expected)!r}, got {sorted(actual)!r}")

    with tempfile.TemporaryDirectory() as tmp:
        makefile_normal = os.path.join(tmp, "Makefile.normal")
        with open(makefile_normal, "w") as f:
            f.write(
                "ADDITIONAL_SOURCES_UNA :=\\\n"
                "../../Libs/Sources/Service.cpp\\\n"
                "Libs/Source/UnaLogger/Logger.cpp\n"
                "ADDITIONAL_SOURCES :=\n"
            )
        check(
            "normal Makefile",
            parse_makefile_sources(makefile_normal),
            {"Libs/Sources/Service.cpp", "Libs/Source/UnaLogger/Logger.cpp"},
        )

        # Same trailing-"\" gotcha as parse_makefile_sources above,
        # making sure we don't regress it.
        makefile_quirk = os.path.join(tmp, "Makefile.quirk")
        with open(makefile_quirk, "w") as f:
            f.write(
                "ADDITIONAL_SOURCES_UNA :=\\\n"
                "Libs/Source/UnaLogger/Logger.cpp\\\n"
                "Libs/Source/Fit/FitCrc.cpp\\\n"
                "ADDITIONAL_SOURCES :=\n"
            )
        check(
            "trailing-backslash-quirk Makefile",
            parse_makefile_sources(makefile_quirk),
            {"Libs/Source/UnaLogger/Logger.cpp", "Libs/Source/Fit/FitCrc.cpp"},
        )

        vcxproj = os.path.join(tmp, "Application.vcxproj")
        with open(vcxproj, "w") as f:
            f.write(
                '<Project><ItemGroup>\n'
                # framework noise, skip it
                '<ClCompile Include="$(TouchGFXReleasePath)\\framework\\source\\x.cpp"/>\n'
                # the sim entry point; Makefile finds this on its own
                '<ClCompile Include="$(ApplicationRoot)\\simulator\\main.cpp"/>\n'
                # generated code, same deal
                '<ClCompile Include="$(ApplicationRoot)\\generated\\simulator\\src\\mainBase.cpp"/>\n'
                '<ClCompile Include="..\\..\\gui\\src\\model\\Model.cpp"/>\n'
                # $(SdkPath) and a pile of dots should land on the same path
                '<ClCompile Include="$(SdkPath)\\Libs\\Source\\UnaLogger\\Logger.cpp"/>\n'
                '<ClCompile Include="..\\..\\..\\..\\Libs\\Sources\\Service.cpp"/>\n'
                '</ItemGroup></Project>\n'
            )
        check(
            "vcxproj",
            parse_vcxproj_sources(vcxproj),
            {"Libs/Source/UnaLogger/Logger.cpp", "Libs/Sources/Service.cpp"},
        )

    if failures:
        print("SELFTEST FAILED:")
        for f in failures:
            print(f"  {f}")
        return 1
    print("Selftest OK.")
    return 0


def main():
    if "--selftest" in sys.argv[1:]:
        return run_selftest()

    projects = sorted(find_projects(), key=lambda p: p[0])
    if not projects:
        print("No project pairs (simulator/gcc/Makefile + simulator/msvs/Application.vcxproj) found.")
        return 1

    failures = 0
    for makefile_path, vcxproj_path in projects:
        label = project_label(makefile_path)
        make_sources = parse_makefile_sources(makefile_path)
        vcxproj_sources = parse_vcxproj_sources(vcxproj_path)

        missing_from_vcxproj = sorted(make_sources - vcxproj_sources)
        extra_in_vcxproj = sorted(vcxproj_sources - make_sources)

        if not missing_from_vcxproj and not extra_in_vcxproj:
            print(f"OK    {label} ({len(make_sources)} sources)")
            continue

        failures += 1
        print(f"DRIFT {label}")
        for path in missing_from_vcxproj:
            print(f"    in Makefile, missing from Application.vcxproj: {path}")
        for path in extra_in_vcxproj:
            print(f"    in Application.vcxproj, missing from Makefile: {path}")

    if failures:
        print(f"\n{failures} of {len(projects)} project(s) have Makefile/vcxproj source drift.")
        return 1

    print(f"\nAll {len(projects)} project(s) have matching Makefile/vcxproj source lists.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
