---
name: una-build-uapp
description: Build a UNA Watch app's .uapp package for the watch with CMake and the arm-none-eabi toolchain, and set its version, ID and memory sizes. Use when building, rebuilding or versioning a UNA Watch app for the device, or when a .uapp build fails.
---

# Build a UNA Watch app (.uapp)

A `.uapp` holds the app's service ELF, its GUI ELF (none for a glance), its icons and a header
with the app's name, type, ID and version. One CMake project builds all of it: the `-CMake`
folder under `Software/Apps` (`Software/App` for glances).

Needs `UNA_SDK` set, `arm-none-eabi-gcc` on `PATH` and the packer's Python packages (skill
`una-setup`).

## Build

From the app root:

```bash
cmake -G Ninja -S Software/Apps/MyApp-CMake -B build
cmake --build build
```

- **Output:** the `.uapp` lands in the project's `OUTPUT_PATH` folder, named
  `<APP_USER_NAME>_<version>.uapp`, for example `MyApp_1.2.0.uapp`. Spaces and other
  characters unsafe in a file name become `_`, so `My App` gives `My_App_1.2.0.uapp`. That is `Output/` in most
  examples and `OutputLVGL/` in the tutorials' LVGL projects.
- **Generator:** `-G "Unix Makefiles"` works with `make` on Linux and macOS. On Windows use Ninja.
- **Reconfiguring:** the version is read from git when CMake configures. After a new commit or
  tag, rerun the `cmake -S ... -B build` step: a plain `cmake --build` keeps the old version.
- **Clean build:** delete `build/` and configure again.

The build log ends with the packer's summary. Check that `ID` matches the app's `APP_ID`:

```text
INFO:root:ID             : 03AD5A741E38A35F
```

## Version

The version comes from git tags on the app's repository, read by
`$UNA_SDK/Utilities/Scripts/build-cube/una-version.sh`:

| Repository state | Version |
|---|---|
| HEAD tagged `apps-v1.2.0` | `1.2.0`, even with uncommitted changes |
| Commits after `apps-v1.2.0` | `1.2.0-<n>-<hash>`, where n is the number of commits since the tag |
| Commits after the tag, plus uncommitted changes | `1.2.0-<n>-<hash>-dirty` |
| No `apps-v*` tag | `0.0.0-dev` |
| Not a git repository | `0.0.0-dev` |

- **Tag form:** the tag must be `apps-v<major>.<minor>.<patch>`. A plain `v1.2.0` isn't read.
- **Setting it directly:** pass `-DBUILD_VERSION=1.2.0` when configuring; it must start with
  `A.B.C`.
- **Range:** each of the three numbers must be 0 to 255.
- **Windows:** the script runs under `bash`, from Git for Windows. If `bash` isn't on `PATH` the
  version falls back to `1.0.0`, or `1.0.0-dirty` with uncommitted changes.
- **Releases:** a store release must be a clean build of a tagged commit (skill
  `una-store-submit`).

## Settings in `CMakeLists.txt`

| Variable | Meaning |
|---|---|
| `APP_NAME` | Names the ELFs and targets. |
| `APP_USER_NAME` | Name shown on the watch, and the base of the `.uapp` file name. The watch shows the first 15 bytes (UTF-8). Defaults to `APP_NAME`. |
| `APP_FILE_NAME` | Base name of the `.uapp`, when it must stay fixed while `APP_USER_NAME` changes. The phone app's update flow keys on the file name. |
| `APP_ID` | 16 uppercase hex digits, unique per app. |
| `APP_TYPE` | `Activity`, `Utility`, `Glance` or `Clockface`, spelled exactly. |
| `DEV_ID` | Developer name, passed to the code as `DEV_ID`. |
| `APP_AUTOSTART` | `On`/`Off` (default `Off`). |
| `APP_USE_ICONS` | `On`/`Off` (default `On`). With `On`, `Resources/icon_30x30.png` and `icon_60x60.png` must exist. Glances set `Off`. |
| `UNA_APP_SERVICE_RAM_LENGTH`, `UNA_APP_GUI_RAM_LENGTH` | Size of each process's linker RAM region (defaults 500K and 600K). It is a ceiling: the kernel allocates only what the image needs. |
| `UNA_APP_SERVICE_STACK_SIZE`, `UNA_APP_GUI_STACK_SIZE` | Stack sizes (default 10 KB). `SERVICE_STACK_SIZE` and `GUI_STACK_SIZE` are accepted too; the LVGL examples set `GUI_STACK_SIZE "24*1024"`. |

Boolean options accept `On/Off/True/False/Yes/No/Y/N/1/0` in any case. Anything else stops the
build.

## Size

The ELFs land in the CMake project's own `build/` folder (`Software/Apps/MyApp-CMake/build/`,
set by `CMAKE_RUNTIME_OUTPUT_DIRECTORY`), not in the `-B` folder. Their map files land in
`Output/` (`MyAppService.elf.elf.map`, `MyAppGUI.elf.elf.map`). To see what the app uses:

```bash
arm-none-eabi-size Software/Apps/MyApp-CMake/build/*.elf
```

The whole image is loaded into RAM: `text` (code and constant data), `data`, `bss` and the
stack all sit in the one region of `UNA_APP_*_RAM_LENGTH`. When they don't fit, the link fails
with "region `RAM' overflowed"; raise the length or shrink the app.

## Troubleshooting

- **`UNA_SDK` / `una-app.cmake` not found, or a failed compiler check:** see the `una-setup`
  skill.
- **`APP_TYPE`/`APP_AUTOSTART` "is not a boolean" or an unknown type:** fix the spelling in
  `CMakeLists.txt`.
- **Missing icon:** add `Resources/icon_30x30.png` and `icon_60x60.png`, or set
  `APP_USE_ICONS Off`.
- **Undefined reference to a libc function** (`sscanf`, `strtod`…): apps link against the
  kernel's C library exports, not a full libc. A function the kernel doesn't export either fails
  to link, or pulls a copy into the app's RAM. Prefer the SDK's helpers, or simpler functions.
- **LVGL app: `lvgl` sources missing:** run `git submodule update --init ThirdParty/lvgl` in the
  SDK.
- **TouchGFX app: missing generated headers:** `generated/` is missing or out of date.
  Regenerate on Windows (skill `una-new-app`).
