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

## Troubleshooting

- If `core_json.h` is missing, confirm include path:
  `SDK/ThirdParty/coreJSON/source/include`.
- If linker errors mention logger symbols, ensure `SDK/Libs/Source/UnaLogger/Logger.cpp` is linked.
- If app headers are not found, add app lib header paths to `target_include_directories(...)` for the test target.
