---
name: una-unit-test
description: Write and run host unit tests (GoogleTest, on the PC's own compiler) for a UNA Watch app's logic, using the SDK's kernel test doubles and in-memory file system. Use when adding tests to a UNA Watch app, testing settings or file code without a watch or simulator, or setting up a Tests/ folder in an app repository.
---

# Unit-test a UNA Watch app

Unit tests build the app's own classes with the PC's compiler (MSVC, GCC or Clang) against the
SDK's fake kernel. They need no ARM toolchain, simulator or watch, and run in seconds. Test
logic here, and use the simulator (skill `una-simulator`) for screens and the whole app.

## What to test

Good candidates are plain classes in `Software/Libs` that take an `SDK::Kernel&` or nothing:
- settings and their JSON serialisation;
- calculations such as pace, zones, laps and alarms;
- state machines;
- anything that reads or writes files.

`Service.cpp` itself, sensor callbacks and GUI code depend on the running kernel, so test them
in the simulator instead. If the logic the user wants tested is tangled into `Service.cpp`,
suggest moving it into its own class first.

## Set up `Tests/` in the app

1. Copy `references/CMakeLists.txt` from this skill to `<app>/Tests/CMakeLists.txt`.
2. In `add_executable(app-tests ...)`, list:
   - the test files;
   - the app sources under test, one by one, from `Software/Libs/Sources` (`Software/Libs/Source`
     in glances; change the template's paths to match).
3. If the code under test uses more SDK sources (for example `Libs/Source/Fit/*.cpp` for FIT
   files), add them to that list too, from `${UNA_SDK}/Libs/Source/...`. A linker error naming
   an SDK symbol means one is missing.
4. Ignore `build-tests/` in git.

## Build and run

From the app root, with `UNA_SDK` set:

```bash
cmake -S Tests -B build-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests --config Debug
ctest --test-dir build-tests -C Debug --output-on-failure
```

- **First configure:** it downloads GoogleTest, so it needs network access.
- **Windows:** the Visual Studio generator is the default. `-C Debug` and `--config Debug` are
  required there and harmless elsewhere.
- **One test:** run the binary directly, for example
  `build-tests/Debug/app-tests --gtest_filter='Settings*'` (`build-tests/app-tests` with
  single-config generators).

## Writing tests

```cpp
#include <gtest/gtest.h>
#include "KernelTestDoubles.hpp"
#include "SettingsSerializer.hpp"   // app code

using SDK::TestSupport::KernelFixture;

TEST(Settings, LoadsSavedUnits)
{
    KernelFixture fixture;   // fixture.kernel is a ready SDK::Kernel
    fixture.fileSystem.seedFile("settings.json", R"({"units":"imperial"})");

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    Settings s{};
    ASSERT_TRUE(serializer.load(s));
    EXPECT_EQ(s.units, Units::Imperial);
}
```

`KernelFixture` holds:
- `system`: `StubSystem`; set `nowMs` to move its clock;
- `logger`;
- `memory`;
- `comm`: `StubAppComm`;
- `fileSystem`: `InMemoryFileSystem`;
- `kernel`: built from all of the above.

### The in-memory file system

- **Seeding and reading:** `seedFile(path, content)` creates a file and the directories along its
  path. `readFile(path)` returns what the code wrote.
- **Listing:** `dir(path)` with `readNext()` lists direct children only, in creation order, not
  name order. A listing is a snapshot taken at `open()`.
- **Paths:** names are case-sensitive. Leading and trailing `/` don't matter. `.` and `..` are
  plain names.
- **Injecting failures:**
  - `failWritesAfterBytes` makes writes fail once that many bytes have been written, across all
    files;
  - `closeGate` decides, per path, whether a `close()` fails;
  - `failWriteOpenSuffix` makes opening a file with that suffix for writing fail;
  - `failRenames` makes every rename fail.
- **Checking file handling:** `openHandles` and `flushCounts` count per path, so a test can check
  that the code closed and flushed its files.
- **Testing a scan:** a test that expects "nothing found" can't tell a correct result from a scan
  that never ran. First assert that the listing returns what was seeded, or pair the test with a
  positive case.
- **Full contract:** `Docs/unit-testing.md` in the SDK lists where the fake differs from the
  watch's FAT file system.

### Rules

- **Keep tests independent of the clock and the machine.** Set times explicitly (`nowMs`, or the
  time a function takes). Don't depend on the PC's time zone.
- **Assert on behaviour the user cares about.** For example, settings survive a save and load,
  and a corrupt file falls back to defaults. Don't assert on private details.
- **A test must be able to fail.** When fixing a bug, write the test first and watch it fail.
- **Watch the integer widths.** The watch is 32-bit and the PC is 64-bit, so `size_t`, `long` and
  pointer sizes differ. Use fixed-width types (`uint32_t`) in app code that is serialised or sent
  in messages.

## Troubleshooting

- **`UNA_SDK environment variable is not set`:** set it in this shell (skill `una-setup`).
- **`core_json.h` not found:** check that `UNA_SDK` points at the SDK root; the header is in
  `$UNA_SDK/ThirdParty/coreJSON/source/include`.
- **Undefined logger symbols:** keep `Libs/Source/UnaLogger/Logger.cpp` in `una_test_support`.
- **App header not found:** add its folder to `target_include_directories(app-tests ...)`.
