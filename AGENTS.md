# AGENTS.md

Guidance for AI coding agents working with the UNA Watch SDK. The SDK builds apps for the UNA
Watch: an STM32U5 (Cortex-M33) watch with a 240 × 240 round display and four buttons, running a
FreeRTOS kernel that loads apps from `.uapp` files.

## Building a watch app

Each stage of building an app has a skill in `skills/`. Read the matching `SKILL.md` before you
start a stage:

| Stage | Skill |
|---|---|
| Install the toolchain, clone the SDK, set `UNA_SDK` | [`una-setup`](skills/una-setup/SKILL.md) |
| Start an app from an example, choose TouchGFX or LVGL | [`una-new-app`](skills/una-new-app/SKILL.md) |
| Unit-test the app's logic on the PC | [`una-unit-test`](skills/una-unit-test/SKILL.md) |
| Run the app in the desktop simulator, scripted and headless | [`una-simulator`](skills/una-simulator/SKILL.md) |
| Build the `.uapp` for the watch | [`una-build-uapp`](skills/una-build-uapp/SKILL.md) |
| Install it on a watch and check it there, unattended with Developer on | [`una-test-on-watch`](skills/una-test-on-watch/SKILL.md) |
| Prepare a release for the app store | [`una-store-submit`](skills/una-store-submit/SKILL.md) |

To use the skills in your own app's repository, or with an agent that doesn't read this file,
see [Docs/agent-skills.md](Docs/agent-skills.md).

### Facts that apply to every app

- **Two processes.** An app is a *service* (background logic, sensors, files) and, unless it is
  a glance, a *GUI*. They are separate ELF files packed into one `.uapp`. They share no memory and
  talk only through messages.
- **Messages over 256 bytes are dropped on the watch.** The kernel's message pools top out at
  256 bytes, and a larger send is dropped silently. The simulator has no such limit, so a message
  that works there can vanish on the watch. Guard each message type with
  `static_assert(sizeof(MyMessage) <= 256)`. The message header is 32 bytes on the watch and 40
  in a 64-bit simulator or test build, so keep the payload within 216 bytes.
- **RAM ceiling.** The service and the GUI each run entirely from RAM: code, data and stack must
  fit in a region set in the app's `CMakeLists.txt` (`UNA_APP_SERVICE_RAM_LENGTH`,
  `UNA_APP_GUI_RAM_LENGTH`), or the link fails. An app's C library
  calls go through the kernel's export table, not a full libc.
- **App types:** `Activity`, `Utility`, `Glance` or `Clockface`, set by `APP_TYPE`.
- **`APP_ID`** is 16 uppercase hex digits and must be unique. The app store issues one for a
  published app.
- **Display colour:** the panel shows 2 bits per channel (64 colours), so pick colours that
  survive that. The TouchGFX simulators preview the panel's tones: press `L`, or set
  `UNA_SIM_LOOK=watch`.
- **Never edit TouchGFX `generated/` folders.** Code generation rewrites them. App code belongs
  in `gui/`.
- **Check on a watch before release.** The simulator runs the real app code against a mock
  kernel. Sensors, timing, memory limits and fonts can differ on the watch.

### Reference

- Examples: `Examples/Apps/` (shipping apps) and `Docs/Tutorials/` (one feature each, with an
  `ARCHITECTURE.md` walkthrough).
- Public headers: `Libs/Header/SDK/`. The kernel's interface is `SDK::Kernel`, which holds
  `ISystem`, `IAppComm`, `IFileSystem`, `ILogger` and `IAppMemory`.
- Docs: `Docs/` (published at the SDK documentation site). Start with `Docs/sdk-setup.md`,
  `Docs/Simulator.md` and `Docs/deploy.md`.

## Changing the SDK itself

For a pull request to this repository:

- **Branch from `main` and rebase.** `main` takes fast-forward merges only.
- **Use Conventional Commit messages** (`feat(simulator): ...`, `fix: ...`, `docs: ...`).
- **Host tests:** `cmake -S Tests/Host -B build-host -DCMAKE_BUILD_TYPE=Debug`, then
  `cmake --build build-host`, then `ctest --test-dir build-host --output-on-failure`. Add
  `-C Debug` to `ctest` on Windows.
- **Code that only the simulators compile:** code under `Libs/Source/Simulator` is built only by
  the app simulators, so build one before opening a PR.
- **Formatting:** match the surrounding code: 4-space indents, a function's opening brace on
  its own line, and braces attached for control statements. Don't run clang-format over existing
  files; the SDK's code doesn't follow `.clang-format`.
- **Header guards:** `#ifndef NAME_HPP` style, with no leading underscores.
- **Two source lists for TouchGFX simulators:** a source added to one must go into both its
  `simulator/msvs/Application.vcxproj` and its `simulator/gcc/Makefile`.
  `python3 .github/scripts/check_msvs_sources.py` checks that they match.
- **Example apps ship to users,** so change them at production quality.
