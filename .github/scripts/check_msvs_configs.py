#!/usr/bin/env python3
"""Two scaffolding checks for every TouchGFX-GUI project, both from #288.

1. Application.vcxproj: the Release ClCompile settings must agree with Debug.
   Release defines exactly Debug's preprocessor definitions (with NDEBUG for
   _DEBUG), and its include path inherits %(AdditionalIncludeDirectories) and
   carries every directory Debug adds. Only Debug was ever maintained, so
   Release quietly lost the app identity macros, the app's Libs/Header and the
   SDK headers from SDK.props, and nobody noticed: CI never builds MSVC, let
   alone its Release configuration.

2. una/Makefile: the object_files assignment must keep the backslash that
   carries it onto the .c mapping line. Without it the second line parses as a
   separate (bogus) rule, which is harmless only while there are no .c files
   under touchgfx_path.

No MSVC or Windows needed; it runs anywhere Python 3 does.
"""
import os
import re
import sys
import tempfile

REPO_ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
SEARCH_ROOTS = ["Examples/Apps", "Docs/Tutorials"]

INHERIT_INCLUDES = "%(AdditionalIncludeDirectories)"
# The .cpp mapping line of the object_files assignment, whatever follows it.
OBJECT_FILES_CPP_LINE = re.compile(r"^object_files := .*touchgfx/%\.o\).*$", re.M)


def find_projects():
    """Yield each TouchGFX-GUI directory that has the simulator scaffolding."""
    for root in SEARCH_ROOTS:
        base = os.path.join(REPO_ROOT, root)
        for dirpath, dirnames, _ in os.walk(base):
            dirnames.sort()
            if os.path.basename(dirpath) == "TouchGFX-GUI":
                yield dirpath
                dirnames[:] = []


def item_definition_group(text, config):
    m = re.search(
        r"<ItemDefinitionGroup Condition=\"'\$\(Configuration\)\|\$\(Platform\)'=='%s\|Win32'\">(.*?)</ItemDefinitionGroup>"
        % config, text, re.S)
    return m.group(1) if m else None


def cl_compile(group):
    """The ClCompile block of an ItemDefinitionGroup, so that settings are never
    read from a ResourceCompile (or other tool) block that happens to precede it."""
    if group is None:
        return None
    m = re.search(r"<ClCompile>(.*?)</ClCompile>", group, re.S)
    return m.group(1) if m else None


def element(block, name):
    m = re.search(r"<%s>([^<]*)</%s>" % (name, name), block)
    return m.group(1) if m else None


def check_vcxproj(text):
    """Return a list of problems with Release/Debug agreement."""
    debug = cl_compile(item_definition_group(text, "Debug"))
    release = cl_compile(item_definition_group(text, "Release"))
    if debug is None or release is None:
        return ["no ClCompile in the Debug|Win32 or Release|Win32 ItemDefinitionGroup"]

    problems = []
    debug_defs = element(debug, "PreprocessorDefinitions") or ""
    release_defs = element(release, "PreprocessorDefinitions") or ""
    # Swap the _DEBUG entry wherever it sits in the list, not only mid-list.
    expected = ";".join("NDEBUG" if d == "_DEBUG" else d for d in debug_defs.split(";"))
    if release_defs != expected:
        problems.append("Release PreprocessorDefinitions differ from Debug's:\n"
                        "        Debug:   %s\n        Release: %s" % (debug_defs, release_defs))

    debug_inc = [e for e in (element(debug, "AdditionalIncludeDirectories") or "").split(";") if e]
    release_inc = [e for e in (element(release, "AdditionalIncludeDirectories") or "").split(";") if e]
    if INHERIT_INCLUDES not in release_inc:
        problems.append("Release AdditionalIncludeDirectories drops %s, losing SDK.props' include"
                        % INHERIT_INCLUDES)
    missing = [e for e in debug_inc if e not in release_inc and e != INHERIT_INCLUDES]
    if missing:
        problems.append("Release AdditionalIncludeDirectories lacks Debug's: %s" % ";".join(missing))
    return problems


def check_makefile(text):
    m = OBJECT_FILES_CPP_LINE.search(text)
    if m is None:
        return ["no object_files := line mapping touchgfx/%.o found"]
    # Make continues a line only on a backslash immediately before the
    # newline; a space after it ends the line just as surely as no backslash.
    if not m.group(0).endswith("\\"):
        return ["object_files := line does not end in a backslash continuation"]
    return []


def check_project(project_dir):
    problems = []
    for rel, checker in (("simulator/msvs/Application.vcxproj", check_vcxproj),
                         ("una/Makefile", check_makefile)):
        path = os.path.join(project_dir, rel)
        if not os.path.isfile(path):
            problems.append("%s: missing" % rel)
            continue
        with open(path, encoding="utf-8-sig") as f:
            problems += ["%s: %s" % (rel, p) for p in checker(f.read())]
    return problems


GOOD_VCXPROJ = """<Project>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'"><X/></PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <ClCompile>
      <PreprocessorDefinitions>WIN32;_DEBUG;APP_ID="X";%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>%(AdditionalIncludeDirectories);../../gui/include;../../../../Libs/Header</AdditionalIncludeDirectories>
    </ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <ClCompile>
      <PreprocessorDefinitions>WIN32;NDEBUG;APP_ID="X";%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>%(AdditionalIncludeDirectories);$(ApplicationRoot)\\gui\\include;../../gui/include;../../../../Libs/Header</AdditionalIncludeDirectories>
    </ClCompile>
  </ItemDefinitionGroup>
</Project>
"""

GOOD_MAKEFILE = """object_files := $(source_files:$(touchgfx_path)/%.cpp=$(object_output_path)/touchgfx/%.o) \\
$(c_source_files:$(touchgfx_path)/%.c=$(object_output_path)/touchgfx/%.o)
"""

# Release's copy is the last one in the fixture; take only that.
_head, _sep, _tail = GOOD_VCXPROJ.rpartition(";../../../../Libs/Header")
RELEASE_LIBS_HEADER_GONE = _head + _tail

# _DEBUG first in Debug's list; Release either swaps it properly or keeps it.
DEBUG_FIRST_RELEASE_OK = GOOD_VCXPROJ.replace("WIN32;_DEBUG;", "_DEBUG;WIN32;").replace(
    "WIN32;NDEBUG;", "NDEBUG;WIN32;")
DEBUG_FIRST_RELEASE_KEEPS_DEBUG = GOOD_VCXPROJ.replace("WIN32;_DEBUG;", "_DEBUG;WIN32;").replace(
    "WIN32;NDEBUG;", "_DEBUG;WIN32;")

# Matching ResourceCompile blocks ahead of ClCompile must not mask a ClCompile
# difference (here, Release losing its identity macro).
RESOURCE_COMPILE_FIRST = GOOD_VCXPROJ.replace(
    "    <ClCompile>\n",
    "    <ResourceCompile>\n"
    "      <PreprocessorDefinitions>RC;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n"
    "      <AdditionalIncludeDirectories>%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n"
    "    </ResourceCompile>\n"
    "    <ClCompile>\n").replace('NDEBUG;APP_ID="X";', "NDEBUG;")


def run_selftest():
    """--selftest runs the checks against fixtures here, one per defect #288
    found, so a checker that silently stops matching fails loudly instead."""
    cases = [
        ("clean project", GOOD_VCXPROJ, GOOD_MAKEFILE, 0),
        ("Release lacks identity macros",
         GOOD_VCXPROJ.replace('NDEBUG;APP_ID="X";', "NDEBUG;"), GOOD_MAKEFILE, 1),
        ("Release lacks Libs/Header", RELEASE_LIBS_HEADER_GONE, GOOD_MAKEFILE, 1),
        ("Release drops inherited includes",
         GOOD_VCXPROJ.replace("%(AdditionalIncludeDirectories);$(ApplicationRoot)", "$(ApplicationRoot)"),
         GOOD_MAKEFILE, 1),
        ("Makefile lost its continuation", GOOD_VCXPROJ, GOOD_MAKEFILE.replace(" \\\n", " \n"), 1),
        ("Makefile has a space after the backslash",
         GOOD_VCXPROJ, GOOD_MAKEFILE.replace(" \\\n", " \\ \n"), 1),
        ("_DEBUG first, Release swaps it", DEBUG_FIRST_RELEASE_OK, GOOD_MAKEFILE, 0),
        ("_DEBUG first, Release keeps it", DEBUG_FIRST_RELEASE_KEEPS_DEBUG, GOOD_MAKEFILE, 1),
        ("ResourceCompile ahead of ClCompile", RESOURCE_COMPILE_FIRST, GOOD_MAKEFILE, 1),
        ("missing Application.vcxproj", None, GOOD_MAKEFILE, 1),
        ("missing una/Makefile", GOOD_VCXPROJ, None, 1),
    ]
    failed = 0
    for name, vcxproj, makefile, expected in cases:
        with tempfile.TemporaryDirectory() as tmp:
            # None leaves that file out, as a project missing its scaffolding.
            for rel, content in (("simulator/msvs/Application.vcxproj", vcxproj),
                                 ("una/Makefile", makefile)):
                if content is None:
                    continue
                path = os.path.join(tmp, rel)
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, "w") as f:
                    f.write(content)
            got = len(check_project(tmp))
        ok = got == expected
        failed += not ok
        print("%s  %s (expected %d problem(s), got %d)" % ("PASS" if ok else "FAIL", name, expected, got))
    return 1 if failed else 0


def main():
    if "--selftest" in sys.argv[1:]:
        return run_selftest()

    bad = 0
    projects = list(find_projects())
    for project in projects:
        label = os.path.relpath(project, REPO_ROOT)
        problems = check_project(project)
        if problems:
            bad += 1
            print("FAIL  %s" % label)
            for p in problems:
                print("      %s" % p)
        else:
            print("OK    %s" % label)

    print()
    if bad:
        print("%d of %d project(s) have simulator scaffolding drift. See #288." % (bad, len(projects)))
        return 1
    print("All %d project(s) have matching Debug/Release settings and an intact Makefile." % len(projects))
    return 0


if __name__ == "__main__":
    sys.exit(main())
