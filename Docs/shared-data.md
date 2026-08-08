# Shared Data Between Apps

`SharedData/` is a directory that every app can read and write. An app reaches it from its
own root as `../SharedData/`.

It exists because some data belongs to the user, not to an app. Stride calibration is the
case the SDK already ships: the Running app measures it outdoors with GNSS, and the
Treadmill app needs it indoors where there is no GNSS.

For app-private files, see the [Files tutorial](Tutorials/Files/ARCHITECTURE.md).

## The path

```cpp
static constexpr const char *kPath = "../SharedData/stride.json";
```

Paths are relative to the app's own root. The whole path, filename included, must fit
`IFileSystem::skMaxPathLen` (256 bytes).

## Who uses it today

| Path | Written by | Read by |
| --- | --- | --- |
| `../SharedData/stride.json` | `OutdoorStrideCalibrator::finalise()` ([`OutdoorStrideCalibrator.hpp:59`](../Libs/Header/SDK/Calibration/OutdoorStrideCalibrator.hpp)) | `StrideLut` ([`StrideLut.hpp:67`](../Libs/Header/SDK/Calibration/StrideLut.hpp)) |
| `../SharedData/stride_trace.csv` | Running app, when tracing is on ([`Running/.../Service.cpp:789`](../Examples/Apps/Running/Software/Libs/Sources/Service.cpp)) | diagnostic only |
| `../SharedData/stride_deleted.json` | Treadmill app, backing up the LUT before deleting it ([`Treadmill/.../Service.cpp:1177`](../Examples/Apps/Treadmill/Software/Libs/Sources/Service.cpp)) | recovery only |

## Create the directory before opening a file in it

`IFileSystem::mkdir()` creates missing parent directories. Opening a file does not. A writer
that skips the `mkdir` works on your watch and fails on one that has never run a calibrating
app:

```cpp
// Ensure the SharedData directory exists (FatFs f_open does not create
// missing parents). "Already exists" counts as success.
const char *slash = std::strrchr(mPath, '/');
if (slash != nullptr) {
    char dir[SDK::Interface::IFileSystem::skMaxPathLen] {};
    std::snprintf(dir, sizeof(dir), "%.*s", static_cast<int>(slash - mPath), mPath);
    if (!mFs.mkdir(dir)) {
        return false;
    }
}
```

[`OutdoorStrideCalibrator.cpp:296-306`](../Libs/Source/Calibration/OutdoorStrideCalibrator.cpp)

## Every read is optional

Your reader has to work on a watch where nothing has written the file yet. That is the
normal state of a new device, not an edge case.

`StrideLut::loadFromFile()` handles it by clearing itself and returning `false` when the
file is absent or will not open
([`StrideLut.cpp:163-175`](../Libs/Source/Calibration/StrideLut.cpp)). The caller gets an
all-zero LUT and falls back to a default model.

## Share only what is shared

App-specific data stays in the app's own root even when it is closely related to something
shared. The Treadmill app keeps its delta-LUT out:

```cpp
/// Delta-LUT filename in the Treadmill app's own root (NOT under SharedData).
```

[`CadenceStrideModelConfig.hpp:72`](../Libs/Header/SDK/Calibration/CadenceStrideModelConfig.hpp)

Ask whether another app would be *right* to read the file, not just curious about it. A
user's stride length is theirs and follows them between apps. A treadmill's calibration
offset only means anything inside the model that produced it.

## Expect interrupted writes

Apps are scheduled independently, and a watch can lose power mid-write. Write so that a torn
file is detectable: keep a backup copy before replacing (the Treadmill app does this with
`stride_deleted.json`), or use a format whose reader rejects a truncated file. The failure
to design against is a reader that silently accepts half a record.

## In the simulator

The simulator's filesystem root is `Output/`
([`Kernel.cpp:17`](../Libs/Source/Simulator/Kernel/Kernel.cpp)). It passes `..` through to
the host filesystem instead of clamping it to that root, so `../SharedData/` lands beside
`Output/` and not inside it. Apps behave correctly. The files are just not where you would
first look for them.
