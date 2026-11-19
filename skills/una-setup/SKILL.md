---
name: una-setup
description: Set up a machine to build UNA Watch apps. Installs an arm-none-eabi toolchain, CMake, a build tool and Python, clones the UNA SDK and sets UNA_SDK, on Windows, macOS, Linux or in Docker. Use when starting UNA Watch development, when a build can't find arm-none-eabi-gcc or una-app.cmake, or when UNA_SDK is unset.
---

# Set up UNA Watch app development

The goal is a shell where these all work and `UNA_SDK` points at an SDK checkout:

```bash
arm-none-eabi-gcc --version   # Arm GNU Toolchain 14.3.Rel1 or later, or ST's GNU Tools for STM32
cmake --version               # 3.21 or later
ninja --version               # or make
python3 --version             # python on Windows
echo "$UNA_SDK"               # absolute path to the una-sdk checkout
```

Check what is already installed before installing anything. Ask before installing software
system-wide or editing shell profiles.

## 1. Get the SDK

```bash
git clone https://github.com/UNAWatch/una-sdk.git
cd una-sdk
git submodule update --init ThirdParty/lvgl   # LVGL GUIs and the glance simulators need it
python3 -m pip install -r Utilities/Scripts/app_packer/requirements.txt
```

TouchGFX-only builds don't need the LVGL submodule.

Where the system Python refuses `pip install` ("externally-managed-environment", as on recent
Debian, Ubuntu and Homebrew Python), use a virtual environment:
1. Install `python3-venv` on Debian and Ubuntu.
2. Run `python3 -m venv ~/.venvs/una`.
3. Install the requirements with `~/.venvs/una/bin/pip install -r ...`.
4. Activate it (`source ~/.venvs/una/bin/activate`) before configuring an app. The build uses the
   first `python3` on `PATH` when it is configured. Alternatively, pass
   `-DUNA_PYTHON_EXECUTABLE=$HOME/.venvs/una/bin/python3`.

GitHub's "Source code" release archives leave out submodules, so an SDK taken from one builds
TouchGFX apps but not LVGL ones. Clone the repository instead. Release tags are `sdk-v*` for the
SDK and `apps-v*` for the example apps.

## 2. Install a toolchain

Use one of these two:

- **Arm GNU Toolchain**, 14.3.Rel1 or later, from
  <https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads>. Take the
  `arm-none-eabi` archive for the host, unpack it anywhere and put its `bin` folder on `PATH`.
  No ST account or software is needed.
- **ST GNU Tools for STM32**, which comes with STM32CubeIDE or STM32CubeCLT.

Do not use a Linux distribution's `gcc-arm-none-eabi` package. It often fails to link, with
missing `_write`, `_close` and similar.

### Per host

| Host | Toolchain archive | Build tools |
|---|---|---|
| Windows | `mingw-w64-x86_64-arm-none-eabi` zip | `pip install cmake ninja`; build with `-G Ninja` |
| macOS (Apple Silicon) | `darwin-arm64-arm-none-eabi` | `brew install cmake ninja` |
| macOS (Intel) | No Arm GNU Toolchain build at 14.3.Rel1 or later: use ST's GNU Tools for STM32 (in STM32CubeCLT), or Docker (below) | `brew install cmake ninja` |
| Linux x86-64 | `x86_64-arm-none-eabi` tar.xz | `apt install cmake ninja-build python3-pip git` (or the distro's equivalent) |
| Linux arm64 | `aarch64-arm-none-eabi` tar.xz | as above |

**Windows:**
- Use `-G Ninja`. With ST's `make`, the "Unix Makefiles" generator can fail CMake's compiler
  test.
- The build runs `bash` to derive the app version from git. Git for Windows provides `bash`;
  without it on `PATH` the version falls back to `1.0.0`.
- The build looks for `python3` before `python`, and a Windows virtual environment has only
  `python.exe`, so CMake can pick a different Python from the one the requirements went into.
  Pass the interpreter when configuring an app:
  `-DUNA_PYTHON_EXECUTABLE="$((Get-Command python).Source)"`.

**macOS:** untested by the SDK maintainers. If something fails, tell the user and suggest
reporting it at <https://github.com/UNAWatch/una-sdk/issues>.

## 3. Set UNA_SDK

`UNA_SDK` is the absolute path of the SDK checkout. Every app's `CMakeLists.txt` and every
simulator build reads it.

```bash
export UNA_SDK="$HOME/path/to/una-sdk"   # bash/zsh; add to ~/.bashrc or ~/.zshrc to keep it
```

```powershell
$env:UNA_SDK = "C:\path\to\una-sdk"                                   # this session
[Environment]::SetEnvironmentVariable("UNA_SDK", "C:\path\to\una-sdk", "User")  # persistent
```

On Windows, `Utilities/Scripts/export-stm32-tools.ps1` sets `UNA_SDK` persistently and finds an
installed STM32CubeIDE toolchain. Dot-source it:
`. <una-sdk>/Utilities/Scripts/export-stm32-tools.ps1`. A persistent variable reaches only
shells opened after it was set.

## 4. Docker

`references/Dockerfile` builds an Ubuntu image with:
- the Arm GNU Toolchain;
- CMake, Ninja and Python with the packer's dependencies;
- SDL2, for the LVGL and glance simulators.

It works on x86-64 and arm64 hosts. From the SDK root:

```bash
docker build -t una-build -f skills/una-setup/references/Dockerfile Utilities/Scripts/app_packer
docker run --rm -it --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:/sdk" -v "/path/to/my-app:/app" una-build
```

On a Linux host, `--user` makes the build output belong to you rather than root. Docker Desktop
on Windows and macOS doesn't need `--user` or `-e HOME=/tmp`.

Inside, `UNA_SDK=/sdk`, and the build commands in the `una-build-uapp` skill work as written.
TouchGFX Designer doesn't run in Docker, so regenerate TouchGFX code on Windows. See the
`una-new-app` skill.

## 5. Verify

Build one example out of the SDK tree, as a new app would be:

```bash
cp -r "$UNA_SDK/Examples/Apps/Alarm" /tmp/MyAlarm
cmake -G Ninja -S /tmp/MyAlarm/Software/Apps/Alarm-CMake -B /tmp/MyAlarm/build
cmake --build /tmp/MyAlarm/build
ls /tmp/MyAlarm/Output    # Alarm_<version>.uapp
```

## Optional tools

| Tool | Needed for | Hosts |
|---|---|---|
| TouchGFX Designer (free ST account) | Editing a TouchGFX GUI and regenerating its code | Windows only |
| Visual Studio, or Build Tools for Visual Studio with "Desktop development with C++" | Simulators on Windows | Windows |
| GCC, SDL2 (`libsdl2-dev`), and for TouchGFX simulators `libsdl2-image-dev libjpeg-dev ruby ruby-nokogiri` | Simulators on Linux | Linux |
| Apple Clang, `brew install sdl2` | LVGL and glance simulators | macOS |
| Node.js | Converting new fonts for an LVGL GUI | all |

## Troubleshooting

- **`include could not find requested file: /cmake/una-app.cmake`:** `UNA_SDK` is not set in
  this shell.
- **`CMAKE_C_COMPILER not set` or a failed compiler test:** `arm-none-eabi-gcc` is not on `PATH`.
  On Windows, use `-G Ninja`. As a last resort, add
  `-DCMAKE_C_COMPILER=arm-none-eabi-gcc -DCMAKE_CXX_COMPILER=arm-none-eabi-g++ -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc`.
- **Undefined `_write`, `_close` and similar:** a distro `gcc-arm-none-eabi` is first on `PATH`.
  Use the Arm GNU or ST toolchain.
- **Stale configuration after changing tools:** delete the build folder and configure again.
