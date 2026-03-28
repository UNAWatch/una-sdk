# UNA SDK — Linux Simulator Compatibility Fixes

This document records the changes made to the SDK to enable the TouchGFX simulator
to build on x86-64 Linux (Ubuntu 24.04, GCC 13). All changes are backward-compatible:
the embedded ARM build is unaffected and Windows simulator builds continue to work.

---

## Summary of changes

| File | Issue | Fix |
|------|-------|-----|
| `Libs/Header/SDK/Simulator/OS/OS.hpp` | `#include <array>` only in `#ifdef _WIN32` block; `OS::Queue<T,N>` uses `std::array` | Add `#include <array>` to `#else` (Linux) branch |
| `Libs/Header/SDK/Simulator/Kernel/Mock/Logger.hpp` | `GetTickCount64()` is Win32-only | Add portable `GetTickCount64()` wrapper using `clock_gettime(CLOCK_MONOTONIC)` for `#ifndef _WIN32` |
| `Libs/Source/Simulator/Kernel/Mock/System.cpp` | `GetTickCount64()` and `Sleep()` are Win32-only | Add local wrappers using `clock_gettime` and `usleep` for `#ifndef _WIN32` |
| `Libs/Header/SDK/Messages/MessageBase.hpp` | `static_assert(sizeof(MessageBase) == 32)` fails on x86-64 where `void*` and vtable pointer are 8 bytes | Guard with `#if __SIZEOF_POINTER__ == 4` |
| `Libs/Header/SDK/Messages/CommandMessages.hpp` | 18 `static_assert` size checks that assume 32-bit pointer layout | Guard each with `#if __SIZEOF_POINTER__ == 4` |
| `Libs/Source/Wrappers/StdLibWrappers.c` | `std::strncpy` used in a `.c` (C) file; `std::` namespace is C++ only | Replace with plain `strncpy` |

---

## Detail

### `OS.hpp` — missing `#include <array>`

`OS::Queue<T, N>` uses `std::array<T, N>` for its internal buffer. The `#include <array>`
was inside the `#ifdef _WIN32` block, so it was never included on Linux, leaving
`std::array` with only a forward declaration (from `<tuple>` pulled in transitively).
This caused an "incomplete type" error when the template was instantiated.

**Fix**: add `#include <array>` to the `#else` block.

---

### `Logger.hpp` and `System.cpp` — `GetTickCount64()` / `Sleep()`

`GetTickCount64()` returns milliseconds since boot on Windows. The POSIX equivalent
is `clock_gettime(CLOCK_MONOTONIC, ...)`. `Sleep(ms)` maps to `usleep(ms * 1000)` on POSIX.

Both are provided as `static inline` wrappers in `#ifndef _WIN32` blocks so they are
only defined when the platform-provided versions are absent.

---

### `MessageBase.hpp` / `CommandMessages.hpp` — pointer-size assertions

The SDK message structs contain a `void* mCompletionSemaphore` field and a vtable pointer,
both of which are 4 bytes on 32-bit ARM and 8 bytes on x86-64. The struct size checks
hardcoded 32-bit sizes (e.g. `sizeof(MessageBase) == 32`).

On x86-64 the structs are larger (e.g. `sizeof(MessageBase) == 40`), but this does not
affect simulator correctness because:
- The GUI and service threads run in the **same process** and share the same memory layout.
- Binary-format IPC (packed structs sent over a wire) is not used in the simulator.

The assertions are valuable for the embedded ARM build where memory layout matters.
They are preserved under `#if __SIZEOF_POINTER__ == 4` so they still fire for ARM GCC.

---

### `StdLibWrappers.c` — `std::strncpy` in a C file

`StdLibWrappers.c` is compiled as C (not C++). The Linux branch used `std::strncpy`
which is a C++ qualified name and is not valid in C. GCC 13 rejects this.

**Fix**: replace with plain `strncpy` (both C and C++ have it in `<string.h>` / `<cstring>`).

---

## What is NOT fixed in the SDK (handled at app level)

`Libs/Source/Simulator/Kernel/Mock/FileSystem.cpp` and the corresponding header use
Win32 file APIs (`GetFileAttributesA`, `CreateDirectoryA`, `FindFirstFileA`, etc.)
throughout the implementation. Rather than a surgical patch, the rowing app provides
a complete POSIX-based replacement at:

```
apps/rowing/Software/Apps/TouchGFX-GUI/simulator_linux/FileSystem.cpp
apps/rowing/Software/Apps/TouchGFX-GUI/simulator_linux/SDK/Simulator/Kernel/Mock/FileSystem.hpp
```

The `config/gcc/app.mk` selects this replacement by:
1. Adding `simulator_linux/` to the front of `ADDITIONAL_INCLUDE_PATHS` (shadows the SDK header).
2. Replacing the SDK `FileSystem.cpp` entry in `ADDITIONAL_SOURCES_UNA` with the local one.
