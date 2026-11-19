(agent-skills)=
# AI Coding Agents

The SDK ships instructions for AI coding agents: a root
[`AGENTS.md`](https://github.com/UNAWatch/una-sdk/blob/main/AGENTS.md) and a set of skills, one
for each stage of building an app. A skill is a folder with a `SKILL.md` (the open agent skills
format), which an agent loads when a task matches its description.

| Skill | What it covers |
|---|---|
| `una-setup` | Toolchain, SDK clone and `UNA_SDK` on Windows, macOS, Linux or Docker |
| `una-new-app` | Starting an app from the closest example; choosing TouchGFX or LVGL |
| `una-unit-test` | GoogleTest host tests for the app's logic, with the SDK's test doubles |
| `una-simulator` | Building the simulator and driving it with scripts, headless |
| `una-build-uapp` | Building the `.uapp`, versions, IDs and memory settings |
| `una-test-on-watch` | Installing over USB, collecting the watch's logs, on-watch checks |
| `una-store-submit` | Preparing a store release: store ID, manifest, `minKernelVersion`, zip |

The skills live in [`skills/`](https://github.com/UNAWatch/una-sdk/tree/main/skills) in the SDK
repository.

## Claude Code

The SDK repository is a Claude Code plugin marketplace. To install the skills for every project:

```text
/plugin marketplace add UNAWatch/una-sdk
/plugin install una-sdk@una-sdk
```

Inside a clone of the SDK, Claude Code also reads `CLAUDE.md`, which points it at `AGENTS.md`.

## Other agents, or a copy in the app's repository

`Utilities/Scripts/agent_skills/install_agent_skills.py` copies the skills where agents look for
them:

| Folder | Read by |
|---|---|
| `.claude/skills/` | Claude Code |
| `.agents/skills/` | Codex, Cursor, Antigravity and other agents that read the shared `.agents` folder |

```bash
# Into an app's repository, for every agent
python3 "$UNA_SDK/Utilities/Scripts/agent_skills/install_agent_skills.py" --target path/to/my-app

# Into your home folder, for every project
python3 "$UNA_SDK/Utilities/Scripts/agent_skills/install_agent_skills.py" --user
```

- **`--agent claude` or `--agent agents`:** fills only one of the two folders.
- **`--user`:** fills `~/.claude/skills` and `~/.agents/skills`. Codex and Cursor read the
  second. Antigravity reads home-folder skills from `~/.gemini/config/skills` instead: copy the
  skills there yourself, or install into each project with `--target`.
- **Updating:** run the script again after updating the SDK. Each run records what it installed
  in `.una-skills.json` beside the skills folder (`.claude/.una-skills.json`,
  `.agents/.una-skills.json`). A skill that still matches the record is updated to the SDK's new
  copy. A skill that was edited locally is left alone and reported; `--force` replaces it.
  `--dry-run` shows what would change. Commit the record with the skills, so a teammate's update
  run can tell edits from older copies.
- **No record:** without `.una-skills.json` (skills installed by hand, or the record deleted),
  the script can't tell an older copy from an edited one, so it leaves every skill that differs
  from the SDK's copy alone. Check those skills for local changes, keep any you want, then run
  with `--force`.
- **Links:** a skill folder that is a symbolic link or junction is never replaced, even with
  `--force`. Replace the link yourself.
- **Skills only:** the script copies skills, not `AGENTS.md`. An app's own `AGENTS.md` can name
  the skills and the SDK it builds against.

## What the agent does and doesn't do

The skills have the agent do the work it can check itself:
- setting up the build;
- copying and renaming an example;
- writing and running unit tests;
- driving the simulator with scripts and reading its screenshots and logs;
- building the `.uapp`;
- preparing a store release.

Steps that need a person stay with the developer, and the skills tell the agent to say so:
- connecting, restarting and wearing the watch;
- checks on the watch itself;
- signing in to the app store, uploading and publishing.

macOS has not been tested by the SDK maintainers. Report problems at
<https://github.com/UNAWatch/una-sdk/issues>.
