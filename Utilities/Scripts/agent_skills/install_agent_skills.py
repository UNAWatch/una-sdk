#!/usr/bin/env python3
"""Copy the UNA SDK's agent skills into a project, or into the user's home.

Each skill is a folder holding a SKILL.md. Agents look for skills in different places:

  claude   .claude/skills/   Claude Code
  agents   .agents/skills/   Codex, Cursor, Antigravity and other agents that read the
                             shared .agents folder

Usage:
  install_agent_skills.py --target PATH [--agent claude|agents|all] [--force] [--dry-run]
  install_agent_skills.py --user        [--agent claude|agents|all] [--force] [--dry-run]

--target installs into a project (an app's repository), --user into ~/.claude/skills and
~/.agents/skills (read by Codex and Cursor; Antigravity reads ~/.gemini/config/skills).

Only the SDK's own skill folders (una-*) are written. A skill already there and different
from the SDK's is left alone unless --force is given, so local edits are not lost. A skill
folder that is a symbolic link or a junction is never replaced.

A replacement is copied into a staging folder beside the skills folder (for example
.claude/.una-skills-una-setup.tmp) and swapped in only once it is complete. If a run is
interrupted, the next run puts back the copy it was replacing, or removes the staging folder.

Run it again after updating the SDK to update the copies.
"""
import argparse
import filecmp
import os
import shutil
import sys
from pathlib import Path

SDK_ROOT = Path(__file__).resolve().parents[3]
SKILLS_DIR = SDK_ROOT / "skills"
AGENT_DIRS = {
    "claude": Path(".claude") / "skills",
    "agents": Path(".agents") / "skills",
}


def sdk_skills():
    """The SDK's skill folders: una-* folders that hold a SKILL.md."""
    if not SKILLS_DIR.is_dir():
        sys.exit(f"error: no skills folder at {SKILLS_DIR}")
    skills = sorted(p for p in SKILLS_DIR.iterdir()
                    if p.is_dir() and p.name.startswith("una-") and (p / "SKILL.md").is_file())
    if not skills:
        sys.exit(f"error: no skills found in {SKILLS_DIR}")
    return skills


def same_tree(a: Path, b: Path) -> bool:
    """True when two folders hold the same files with the same contents."""
    cmp = filecmp.dircmp(a, b)
    # common_funny: one side has a file where the other has a folder.
    if cmp.left_only or cmp.right_only or cmp.common_funny or cmp.funny_files:
        return False
    _, mismatch, errors = filecmp.cmpfiles(a, b, cmp.common_files, shallow=False)
    if mismatch or errors:
        return False
    return all(same_tree(a / d, b / d) for d in cmp.common_dirs)


def is_link(path: Path) -> bool:
    """A symbolic link or a Windows junction (or anything else that resolves elsewhere)."""
    if path.is_symlink():
        return True
    is_junction = getattr(path, "is_junction", None)   # Python 3.12+
    if is_junction is not None and is_junction():
        return True
    if not os.path.lexists(path):
        return False
    return os.path.realpath(path) != os.path.join(os.path.realpath(path.parent), path.name)


def remove(path: Path) -> None:
    """Remove a file, a link or a folder tree, whichever path is."""
    if is_link(path) or not path.is_dir():
        if path.is_dir() and not path.is_symlink():
            os.rmdir(path)          # a junction: removes the link, not its target
        else:
            path.unlink()
    else:
        shutil.rmtree(path)


def staging_for(dest: Path) -> Path:
    """Where a replacement for dest is built: beside the skills folder, so agents never read
    it as a skill."""
    return dest.parent.parent / f".una-skills-{dest.name}.tmp"


def recover(dest: Path, dry_run: bool):
    """Deal with what an interrupted run left behind. Returns a report line ('' if nothing
    was restored) and the folder that now stands for dest: in a dry run, the copy that a real
    run would put back."""
    staging = staging_for(dest)
    if not os.path.lexists(staging):
        return "", dest
    old = staging / "old"
    if not os.path.lexists(dest) and staging.is_dir() and old.is_dir():
        if not dry_run:
            dest.parent.mkdir(parents=True, exist_ok=True)
            old.rename(dest)
            remove(staging)
            return f"restored {dest} (from an interrupted run)", dest
        return f"restored {dest} (from an interrupted run)", old
    if not dry_run:
        remove(staging)
    return "", dest


def replace_tree(skill: Path, dest: Path) -> None:
    """Replace dest with a copy of skill. The old copy is removed only once the new one is in
    place, and put back if the swap fails."""
    staging = staging_for(dest)
    staging.mkdir()
    new, old = staging / "new", staging / "old"
    try:
        shutil.copytree(skill, new)
        dest.rename(old)
        try:
            new.rename(dest)
        except OSError:
            old.rename(dest)
            raise
    finally:
        if not os.path.lexists(dest) and old.is_dir():
            old.rename(dest)
        try:
            remove(staging)
        except OSError:
            pass   # cleaned up by the next run (recover)


def install(skill: Path, dest_root: Path, force: bool, dry_run: bool) -> list:
    dest = dest_root / skill.name
    restored, present = recover(dest, dry_run)
    lines = [restored] if restored else []
    if is_link(present):
        if present.exists() and same_tree(skill, present):
            return lines + [f"current  {dest}"]
        return lines + [f"skipped  {dest} (a symbolic link or junction; replace it yourself)"]
    if os.path.lexists(present):
        if not present.is_dir():
            return lines + [f"skipped  {dest} (exists and is not a folder)"]
        if same_tree(skill, present):
            return lines + [f"current  {dest}"]
        if not force:
            return lines + [f"skipped  {dest} (differs from the SDK's copy; --force replaces it)"]
        if not dry_run:
            replace_tree(skill, dest)
        return lines + [f"replaced {dest}"]
    if not dry_run:
        dest_root.mkdir(parents=True, exist_ok=True)
        try:
            shutil.copytree(skill, dest)
        except FileExistsError:
            raise   # made by someone else since the check: not ours to remove
        except OSError:
            if os.path.lexists(dest):
                shutil.rmtree(dest, ignore_errors=True)
            raise
    return lines + [f"added    {dest}"]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="Copy the UNA SDK's agent skills into a project or the user's home.")
    where = parser.add_mutually_exclusive_group(required=True)
    where.add_argument("--target", type=Path, help="project folder to install into")
    where.add_argument("--user", action="store_true",
                       help="install into the user's home (~/.claude/skills, ~/.agents/skills)")
    parser.add_argument("--agent", choices=["claude", "agents", "all"], default="all",
                        help="which skills folder to fill (default: all)")
    parser.add_argument("--force", action="store_true",
                        help="replace skills that differ from the SDK's copies")
    parser.add_argument("--dry-run", action="store_true", help="print what would change")
    args = parser.parse_args(argv)

    if args.target is not None:
        base = args.target.expanduser().resolve()
        if not base.is_dir():
            parser.error(f"--target {args.target} is not a folder")
    else:
        base = Path.home()

    agents = list(AGENT_DIRS) if args.agent == "all" else [args.agent]
    skills = sdk_skills()
    skipped = failed = 0
    for agent in agents:
        dest_root = base / AGENT_DIRS[agent]
        for skill in skills:
            try:
                lines = install(skill, dest_root, args.force, args.dry_run)
            except OSError as e:
                lines = [f"failed   {dest_root / skill.name} ({e})"]
            for line in lines:
                skipped += line.startswith("skipped")
                failed += line.startswith("failed")
                print(line)

    if args.dry_run:
        print("dry run: nothing was written")
    if skipped:
        print(f"{skipped} skill folder(s) skipped; see above", file=sys.stderr)
    if failed:
        print(f"{failed} skill folder(s) failed; see above", file=sys.stderr)
    return 1 if skipped or failed else 0


if __name__ == "__main__":
    sys.exit(main())
