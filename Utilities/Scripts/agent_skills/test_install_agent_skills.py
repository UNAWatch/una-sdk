#!/usr/bin/env python3
"""Self-contained tests for install_agent_skills.py (run: python test_install_agent_skills.py).

Installs the SDK's real skills into throwaway folders and checks the copies, the skip of a
locally edited skill, updates recorded in .una-skills.json, --force, --dry-run, --agent,
links and junctions, and recovery from an interrupted or failed replacement. No pytest dependency."""
import contextlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SCRIPT = HERE / "install_agent_skills.py"
SKILLS = HERE.parents[2] / "skills"
failures = []


def run(*args):
    return subprocess.run([sys.executable, str(SCRIPT), *args], capture_output=True, text=True)


def expect(cond, msg):
    print(("ok:   " if cond else "FAIL: ") + msg)
    if not cond:
        failures.append(msg)


def skill_names():
    return sorted(p.name for p in SKILLS.iterdir()
                  if p.name.startswith("una-") and (p / "SKILL.md").is_file())


def main():
    names = skill_names()
    expect(len(names) >= 7, f"the SDK has its skills ({len(names)} found)")

    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)

        r = run("--target", str(proj))
        expect(r.returncode == 0, "first install exits 0")
        for agent in (".claude", ".agents"):
            got = sorted(p.name for p in (proj / agent / "skills").iterdir())
            expect(got == names, f"{agent}/skills holds every skill")
        a = (SKILLS / names[0] / "SKILL.md").read_bytes()
        b = (proj / ".claude" / "skills" / names[0] / "SKILL.md").read_bytes()
        expect(a == b, "a copied SKILL.md matches the SDK's")

        r = run("--target", str(proj))
        expect(r.returncode == 0 and "added" not in r.stdout and "current" in r.stdout,
               "a second install reports every skill current")

        edited = proj / ".claude" / "skills" / names[0] / "SKILL.md"
        edited.write_text("local edit\n", encoding="utf-8")
        r = run("--target", str(proj))
        expect(r.returncode == 1 and "skipped" in r.stdout, "an edited skill is skipped, exit 1")
        expect(edited.read_text(encoding="utf-8") == "local edit\n", "the edit is kept")

        extra = proj / ".agents" / "skills" / names[0] / "notes.md"
        extra.write_text("mine\n", encoding="utf-8")
        r = run("--target", str(proj), "--agent", "agents")
        expect(r.returncode == 1 and extra.exists(), "an added file counts as an edit")

        r = run("--target", str(proj), "--force", "--dry-run")
        expect(r.returncode == 0 and "replaced" in r.stdout, "--dry-run reports the replacement")
        expect(edited.read_text(encoding="utf-8") == "local edit\n", "--dry-run writes nothing")

        r = run("--target", str(proj), "--force")
        expect(r.returncode == 0, "--force exits 0")
        expect(edited.read_bytes() == a, "--force restores the SDK's copy")
        expect(not extra.exists(), "--force removes files the SDK's copy doesn't have")
        expect(not list(proj.rglob(".una-skills-*")), "--force leaves no staging folder behind")

    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        odd = proj / ".claude" / "skills" / names[0] / "SKILL.md"
        odd.mkdir(parents=True)   # a folder where the skill has a file
        r = run("--target", str(proj), "--agent", "claude")
        expect(r.returncode == 1 and odd.is_dir(), "a folder in place of a file counts as an edit")
        r = run("--target", str(proj), "--agent", "claude", "--force")
        expect(r.returncode == 0 and odd.is_file(), "--force repairs it")

    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        r = run("--target", str(proj), "--agent", "claude")
        expect(r.returncode == 0 and (proj / ".claude" / "skills").is_dir()
               and not (proj / ".agents").exists(), "--agent claude fills only .claude")

        r = run("--target", str(proj / "missing"))
        expect(r.returncode == 2, "a missing --target is an error")

    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        elsewhere = proj / "elsewhere"
        elsewhere.mkdir()
        (elsewhere / "SKILL.md").write_text("someone else's\n", encoding="utf-8")
        link = proj / ".claude" / "skills" / names[0]
        link.parent.mkdir(parents=True)
        made = False
        if os.name == "nt":
            # A junction needs no special privilege on Windows.
            made = subprocess.run(["cmd", "/c", "mklink", "/J", str(link), str(elsewhere)],
                                  capture_output=True).returncode == 0
        else:
            try:
                link.symlink_to(elsewhere, target_is_directory=True)
                made = True
            except OSError:
                pass
        if not made:
            print("skip: could not make a link here")
        else:
            r = run("--target", str(proj), "--agent", "claude", "--force")
            expect(r.returncode == 1 and "symbolic link or junction" in r.stdout,
                   "a linked skill folder is skipped, even with --force")
            expect((elsewhere / "SKILL.md").read_text(encoding="utf-8") == "someone else's\n",
                   "the link's target is left alone")
            expect(not list(proj.rglob(".una-skills-*")), "no staging folder is left")

    # What an interrupted replacement leaves: the user's copy in staging, nothing in place.
    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        old = proj / ".claude" / f".una-skills-{names[0]}.tmp" / "old"
        old.mkdir(parents=True)
        (old / "SKILL.md").write_text("my edit\n", encoding="utf-8")
        r = run("--target", str(proj), "--agent", "claude", "--dry-run")
        mine = [line.split()[0] for line in r.stdout.splitlines()
                if line.split(" (")[0].endswith(names[0])]
        expect(mine == ["restored", "skipped"] and r.returncode == 1 and old.is_dir(),
               "--dry-run reports the restore and the edit, and changes nothing")
        r = run("--target", str(proj), "--agent", "claude")
        dest = proj / ".claude" / "skills" / names[0] / "SKILL.md"
        expect("restored" in r.stdout and dest.read_text(encoding="utf-8") == "my edit\n",
               "an interrupted replacement's copy is put back")
        expect(r.returncode == 1 and "skipped" in r.stdout,
               "the restored copy then counts as an edit")
        expect(not old.parent.exists(), "the staging folder is removed")

    # A staging leftover that is a file, beside an installed skill.
    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        run("--target", str(proj), "--agent", "claude")
        junk = proj / ".claude" / f".una-skills-{names[0]}.tmp"
        junk.write_text("junk\n", encoding="utf-8")
        r = run("--target", str(proj), "--agent", "claude", "--force")
        expect(r.returncode == 0 and not junk.exists(), "a leftover staging file is removed")

    # A swap that fails part-way leaves the installed copy and no staging, and the other
    # skills are still installed.
    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        run("--target", str(proj), "--agent", "claude")
        edited = proj / ".claude" / "skills" / names[0] / "SKILL.md"
        edited.write_text("local edit\n", encoding="utf-8")
        later = proj / ".claude" / "skills" / names[-1]
        shutil.rmtree(later)
        sys.dont_write_bytecode = True   # leave no __pycache__ beside the script
        sys.path.insert(0, str(HERE))
        import install_agent_skills as tool
        real_rename = Path.rename

        def failing_rename(self, target):
            if self.name == names[0]:
                raise PermissionError("in use")
            return real_rename(self, target)

        Path.rename = failing_rename
        try:
            out = io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
                code = tool.main(["--target", str(proj), "--agent", "claude", "--force"])
        finally:
            Path.rename = real_rename
        expect(code == 1 and "failed" in out.getvalue(), "a failed swap is reported, exit 1")
        expect(edited.read_text(encoding="utf-8") == "local edit\n", "the installed copy is kept")
        expect(not list(proj.rglob(".una-skills-*")), "a failed swap leaves no staging folder")
        expect((later / "SKILL.md").is_file(), "the other skills are still installed")

    # After an SDK update: a skill installed from the older SDK and never edited is updated.
    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        run("--target", str(proj), "--agent", "claude")
        installed = proj / ".claude" / "skills" / names[0]
        record_file = proj / ".claude" / ".una-skills.json"
        recorded = json.loads(record_file.read_text(encoding="utf-8"))["skills"]
        expect(sorted(recorded) == names, "the install record lists every skill")
        (installed / "SKILL.md").write_text("older version\n", encoding="utf-8")
        recorded[names[0]] = tool.tree_hash(installed)   # as if installed from an older SDK
        record_file.write_text(json.dumps({"skills": recorded}), encoding="utf-8")
        r = run("--target", str(proj), "--agent", "claude")
        expect(r.returncode == 0 and f"updated  {installed}" in r.stdout,
               "an unedited skill from an older SDK is updated without --force")
        expect((installed / "SKILL.md").read_bytes() == (SKILLS / names[0] / "SKILL.md").read_bytes(),
               "the update installs the SDK's copy")
        recorded = json.loads(record_file.read_text(encoding="utf-8"))["skills"]
        expect(recorded[names[0]] == tool.tree_hash(SKILLS / names[0]),
               "the record holds the updated copy")

    # A checkout that turned LF into CRLF still matches the record.
    with tempfile.TemporaryDirectory() as tmp:
        a_dir, b_dir = Path(tmp) / "a", Path(tmp) / "b"
        a_dir.mkdir()
        b_dir.mkdir()
        (a_dir / "SKILL.md").write_bytes(b"one\ntwo\n")
        (b_dir / "SKILL.md").write_bytes(b"one\r\ntwo\r\n")
        expect(tool.tree_hash(a_dir) == tool.tree_hash(b_dir), "the record ignores CRLF vs LF")
        (b_dir / "SKILL.md").write_bytes(b"one\r\nthree\r\n")
        expect(tool.tree_hash(a_dir) != tool.tree_hash(b_dir), "the record still sees an edit")

    # Copies installed before the record existed are recorded once they match the SDK's.
    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        run("--target", str(proj), "--agent", "claude")
        record_file = proj / ".claude" / ".una-skills.json"
        record_file.unlink()
        r = run("--target", str(proj), "--agent", "claude")
        expect(r.returncode == 0 and record_file.is_file()
               and sorted(json.loads(record_file.read_text(encoding="utf-8"))["skills"]) == names,
               "a run with no record writes one for the matching copies")
        record_file.write_text("not json", encoding="utf-8")
        r = run("--target", str(proj), "--agent", "claude")
        expect(r.returncode == 0, "an unreadable record is treated as empty")

    with tempfile.TemporaryDirectory() as tmp:
        proj = Path(tmp)
        r = run("--target", str(proj), "--dry-run")
        expect(r.returncode == 0 and "added" in r.stdout and not any(proj.iterdir()),
               "--dry-run into an empty project writes nothing")

    r = run()
    expect(r.returncode == 2, "--target or --user is required")

    if failures:
        print(f"\n{len(failures)} failure(s)")
        return 1
    print("\nall passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
