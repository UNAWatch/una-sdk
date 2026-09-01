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
file is enough — the directories along its path are implied, with no `mkdir`
needed:

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
  directory `archive` when listing `Activity`, not as a file — the same shape
  a real backend reports.
- **`isDir` is real**, so an `if (item.isDir) continue;` guard is actually
  exercised rather than silently dead.
- **Enumeration is sorted by name**, so tests are reproducible. The device
  enumerates in directory-entry order, so do not write tests that depend on
  alphabetical order *meaning* anything — only on it being stable.
- **A snapshot is taken at `open()`.** Files seeded mid-scan do not appear
  until an explicit `readNext(item, /*reset=*/true)`, which rewinds without
  reading an entry (and re-snapshots, as POSIX `rewinddir` does).
- **`mkdir()` creates parents** and succeeds if the directory already exists;
  `IDirectory::create()` does not, matching the simulator's non-recursive
  `::mkdir`. `remove()` on a directory refuses unless it is empty.
- **A name from a listing is openable as `"/" + name`**, and a listing never
  reports the same name twice — for every name short enough to survive
  `ObjectInfo::name`. Both guarantees stop at that boundary; see the
  name-length divergence below.
- Opening a directory that does not exist fails, so assert on `open()`.

### Prove the scan is live

A test whose expectation is "nothing was found" cannot tell a correct
decision from a scan that never ran — seed a path the fake resolves
differently than you assumed and it still passes. Where the expected outcome
is a negative, assert first that the directory really enumerates what you
seeded, or pair the test with a positive control that differs in exactly the
one property under test.

### Divergences from a real backend

All deliberate. Check these before writing a test that leans on one:

- **An implied directory is only as durable as its contents.** A directory
  that exists solely because a file lives under it stops existing when that
  file is removed. Call `mkdir()` if a test needs it to outlive its children.
- **Leading and trailing slashes are not significant.** `/a.txt`, `a.txt` and
  `a.txt/` are one object, at every depth, for lookup as well as enumeration.
  This is what lets a test seed `/App.uapp` and have code that scans `/` find
  it, but a real filesystem would keep them apart. Repeated *interior*
  separators are not a divergence: `a//b` and `a/b` are one object here as
  they are on POSIX and FatFs.
- **A file may be created under a directory that does not exist.** Writing
  `a/b/c.txt` implies `a` and `a/b` rather than failing with `ENOENT` — the
  flip side of implied directories. `mkdir()` the parents first if a test
  needs the `ENOENT` branch.
- **No `.` / `..` resolution.** Both are ordinary path segments, so `a/b` and
  `a/./b` are different places.
- **Directory rename is not modelled** — it returns false. The simulator's
  does work, so do not read that false as device behaviour.
- **No hidden/system attributes of their own.** `isHidden` is derived from a
  leading `.` in the name, as the POSIX simulator does; `isSystem` is always
  false.
- **A name longer than `ObjectInfo::name` cannot round-trip**; it comes back
  clipped, as the simulator's `safe_strcpy` would clip it. Real backends cap
  a single name well below that. Names are deduplicated *before* clipping, so
  two names differing only past that capacity come back as two identical
  entries — the one case where a listing repeats a name.

What the fake refuses, so a test cannot build a state the device cannot
reach: one name is never both a file and a directory (`mkdir()`,
`IDirectory::create()`, a write-`open()` and `seedFile()` all enforce this);
`rename()`/`copy()` will not take a removed path as their source, nor a
destination that is the root, an existing directory, or under a file; and
`close()` on a handle that is not open fails, so `EXPECT_TRUE(h->close())` is
a real assertion.

## Troubleshooting

- If `core_json.h` is missing, confirm include path:
  `SDK/ThirdParty/coreJSON/source/include`.
- If linker errors mention logger symbols, ensure `SDK/Libs/Source/UnaLogger/Logger.cpp` is linked.
- If app headers are not found, add app lib header paths to `target_include_directories(...)` for the test target.
