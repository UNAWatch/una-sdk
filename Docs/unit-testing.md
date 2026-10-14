# SDK Host Unit Testing

This guide describes the reusable host-side unit test harness for SDK code and app example libraries.

## Layout

```text
SDK/Tests/Host/
├── CMakeLists.txt
├── cmake/
│   └── FetchGoogleTest.cmake
├── support/
│   ├── KernelTestDoubles.hpp
│   └── KernelTestDoubles.cpp
└── apps/
    └── Running/
        ├── Settings_test.cpp
        └── SettingsSerializer_test.cpp
```

- `cmake/`: third-party bootstrap (GoogleTest).
- `support/`: reusable fakes and fixtures (kernel facade dependencies, in-memory filesystem).
- `apps/<AppName>/`: app-specific test cases.

## Build and Run

From SDK root:

```bash
cmake -S Tests/Host -B build-host -DCMAKE_BUILD_TYPE=Debug
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

On Windows multi-config generators:

```powershell
ctest --test-dir build-host -C Debug --output-on-failure
```

Expected: `una-sdk-host-tests` passes.

## Add Tests For Another App

1. Create a folder under `Tests/Host/apps/<YourApp>/`.
2. Add one or more `*_test.cpp` files.
3. Add test files and required app sources to `Tests/Host/CMakeLists.txt` executable source list.
4. Reuse support doubles from `support/KernelTestDoubles.*` when the code requires `SDK::Kernel` or filesystem interactions.

## Serializer/Settings Test Pattern

Use `SDK::TestSupport::KernelFixture`:

- `fixture.kernel` provides a ready `SDK::Kernel` with stubbed dependencies.
- `fixture.fileSystem.seedFile(path, json)` seeds in-memory input files.
- Instantiate serializer with `SettingsSerializer(fixture.kernel, "settings.json")`.
- Verify load/save behavior without device firmware or simulator runtime.

## Testing Code That Scans A Directory

`InMemoryFileSystem` enumerates what you seed, so code that walks a directory
(`kernel.fs.dir(path)` then `readNext()`) can be tested directly. Seeding a
file implies the directories along its path, with no `mkdir` needed:

```cpp
KernelFixture fixture;
fixture.fileSystem.seedFile("Activity/morning.fit", contents);
fixture.fileSystem.seedFile("Activity/archive/old.fit", contents);

auto dir = fixture.kernel.fs.dir("Activity");
ASSERT_TRUE(dir->open());
SDK::Interface::IFileSystem::ObjectInfo item{};
while (dir->readNext(item)) {
    // "archive" (isDir true), then "morning.fit" (isDir false)
}
dir->close();
```

Worth knowing:

- **Direct children only.** `Activity/archive/old.fit` shows up as the
  directory `archive` when listing `Activity`.
- **Enumeration is sorted by name**, so tests are reproducible. The device
  makes no such promise, so rely on the order being stable, not on what it is.
- **`mkdir()` creates parents** and succeeds if the directory already exists;
  `IDirectory::create()` does not create parents.
- **A name from a listing is openable as `"/" + name`**, and a listing never
  reports the same name twice, for every name that fits `ObjectInfo::name`.
- Opening a directory that does not exist fails, so assert on `open()`.

### Prove the scan is live

A test whose expectation is "nothing was found" cannot tell a correct
decision from a scan that never ran. Where the expected outcome is a
negative, assert first that the directory really enumerates what you seeded,
or pair the test with a positive control that differs in exactly the one
property under test.

### Divergences from a real backend

Check these before writing a test that leans on one:

- **A listing is a snapshot taken at `open()`.** Files seeded mid-scan appear
  only after `readNext(item, /*reset=*/true)`, which rewinds without reading
  an entry.
- **An implied directory is only as durable as its contents.** It stops
  existing when its last file is removed. Call `mkdir()` if a test needs it
  to outlive its children.
- **Leading and trailing slashes are not significant**, and runs of `/`
  collapse: `/a.txt`, `a.txt`, `a.txt/` are one object, as are `a//b` and
  `a/b`.
- **The instrumentation is keyed the same way.** `openHandles`,
  `flushCounts` and the `closeGate` argument use the path without leading or
  trailing slashes, so index them as `"Activity/a.fit"`, never
  `"/Activity/a.fit"`.
- **Names are case-sensitive.** `Walk.uapp` and `WALK.UAPP` are two files.
- **A file may be created under a directory that does not exist.** `mkdir()`
  the parents first if a test needs that failure.
- **No `.` / `..` resolution.** Both are ordinary path segments.
- **Directory rename is not modelled**; it returns false.
- **`isHidden` is derived from a leading `.`** in the name; `isSystem` is
  always false.
- **A name longer than `ObjectInfo::name` comes back clipped**, and two names
  differing only past that length come back as identical entries. FatFs caps
  a long file name at 255 characters, which fits, so only a test can seed
  such a name. A 255-character name is real, and cannot be joined onto any
  directory within `skMaxPathLen`.

The fake refuses: one name as both a file and a directory (`mkdir()`,
`IDirectory::create()`, a write-`open()` and `seedFile()` all enforce this);
a removed path as the source of `rename()` or `copy()`; a destination that is
the root, an existing directory, or under a file; a `rename()` onto an
existing file, which FatFs `f_rename` also refuses; removing a directory that
is not empty; and `close()` on a handle that is not open, so
`EXPECT_TRUE(h->close())` is a real assertion. `failRenames` makes every
`rename()` fail.

## Troubleshooting

- If `core_json.h` is missing, confirm include path:
  `SDK/ThirdParty/coreJSON/source/include`.
- If linker errors mention logger symbols, ensure `SDK/Libs/Source/UnaLogger/Logger.cpp` is linked.
- If app headers are not found, add app lib header paths to `target_include_directories(...)` for the test target.
