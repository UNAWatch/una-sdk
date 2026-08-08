# Shared Data Between Apps

`SharedData/` is a directory that every app can read and write. An app reaches it from its
own root as `../SharedData/`.

It exists because some data belongs to the user, not to an app. Stride calibration is the
case the SDK already ships: the Running app measures it outdoors with GNSS, and the
Treadmill app needs it indoors where there is no GNSS.

On the watch the directory sits beside the app directories rather than inside any of them —
`Apps/SharedData/`, a sibling of `Apps/Running/` and the rest — and it is visible over USB
mass storage, so you can read a shared file from a desktop or seed one by hand.

For app-private files, see the [Files tutorial](Tutorials/Files/ARCHITECTURE.md).

## The path

```cpp
static constexpr const char *kPath = "../SharedData/stride.json";
```

Paths are relative to the app's own root. The whole path, filename included, must fit
`IFileSystem::skMaxPathLen` (256 bytes).

`../SharedData/` is the only path that may leave your app's root, and it is a whitelist
rather than general parent traversal. The kernel resolves `../SharedData` and
`../SharedData/<name>` to the shared directory, and rejects every other `..` segment
outright — leading, embedded or trailing. `../SomethingElse/file` does not fail when you
open it; it never resolves at all. The name is reserved by that rule, so an app cannot
create a sibling directory that collides with it.

Nested paths inside it do work. `../SharedData/maps/uk.map` resolves, and
`mkdir("../SharedData/maps")` creates both levels.

## Who uses it today

| Path | Written by | Read by |
| --- | --- | --- |
| `../SharedData/stride.json` | `OutdoorStrideCalibrator::finalise()` ([`OutdoorStrideCalibrator.hpp:59`](../Libs/Header/SDK/Calibration/OutdoorStrideCalibrator.hpp)) | `StrideLut` ([`StrideLut.hpp:67`](../Libs/Header/SDK/Calibration/StrideLut.hpp)) |
| `../SharedData/stride.json.bak` | `OutdoorStrideCalibrator`, when the store will not parse ([`OutdoorStrideCalibrator.cpp:222-228`](../Libs/Source/Calibration/OutdoorStrideCalibrator.cpp)) | recovery only |
| `../SharedData/stride_trace.csv` | Running app, when tracing is on ([`Running/.../Service.cpp:789`](../Examples/Apps/Running/Software/Libs/Sources/Service.cpp)) | diagnostic only |
| `../SharedData/stride_deleted.json` | Treadmill app, backing up the LUT before a user-initiated clear ([`Treadmill/.../Service.cpp:1177`](../Examples/Apps/Treadmill/Software/Libs/Sources/Service.cpp)) | recovery only |

The last three are conditional — a watch that has never hit a corrupt store, never enabled
tracing and never cleared its calibration holds only `stride.json`. Expect the others to
appear, but do not require them.

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

One caveat, and it bites precisely where this rule tells you to stop looking. `IFile::open()`
returns a plain `bool`. File absent, file locked by another app's write, timed out waiting
for the volume, and too many files already open all arrive as the same `false`. A reader
that treats every failure as "nothing has written this yet" will quietly mistake *another
app is writing it right now* for *no data exists*, and fall back to defaults.

`StrideLut::loadFromFile()` does exactly that, and for stride calibration the cost is one
session on a default model — a fair trade. If your shared file is larger or more expensive
to regenerate, decide deliberately whether a failed read means "empty" or "try again later".
The filesystem will not tell you which it was.

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

## Concurrent access

Two apps writing the same shared file cannot corrupt it, and cannot both hold it open.

Filesystem calls are serialised — every operation takes a mutex before touching the media,
so writes queue rather than interleave. On top of that the filesystem keeps a table of open
files: it refuses a second open of anything already open for writing, and refuses a
write-mode open of anything open at all. The loser simply gets `false` from `open()`.

That table is small, and it is shared by the entire watch. The system log, the activity
recorder, settings, and every process of every running app draw on the same budget of ten
simultaneously open files. The eleventh open fails no matter who asks. An app that holds
several shared files open at once is competing with everything else the watch is doing, and
whether it fails depends on what that happens to be. Open what you need, use it, close it.

## Expect interrupted writes

Apps are scheduled independently, and a watch can lose power mid-write.

The obvious defence is not available: **`rename()` will not replace an existing file.**
Renaming onto a name already in use fails, so the POSIX idiom of writing a temporary file
and renaming it over the original does not work here. You would have to remove the target
first, which reopens the very window the idiom exists to close.

What the SDK does instead is rotate and fall back. `RecordingMarker` writes a temporary
file, moves the current good file aside to `.bak` (clearing any stale `.bak` first, because
of the rename rule above), renames the temporary into place, and has its reader fall back to
the `.bak` when the primary will not parse
([`RecordingMarker.cpp:82-140`](../Libs/Source/Fit/RecordingMarker.cpp)). A crash at any
single step leaves at least one intact copy. Copy that shape when the file matters.

Be clear-eyed about what the calibration store does *not* do. `OutdoorStrideCalibrator`
overwrites `stride.json` in place, truncating on open, so a power loss mid-write leaves a
torn file and no backup. The `.bak` in the table above is written later — on a subsequent
load, once the store has already failed to parse. That is recovery evidence, not protection.
`stride_deleted.json` is not protection either: the Treadmill app writes it before an
explicit user-initiated *clear calibration*, never before a routine save.

The failure to design against is a reader that silently accepts half a record.

## Lifetime

Shared files outlive the app that wrote them. `SharedData/` is a sibling of the app
directories rather than a child of any one of them, so removing an app cannot take it along.
That is the point — a user's stride calibration should survive reinstalling Running.

Nothing ever collects the garbage. There is no owner, no reference count, and no screen that
lists shared files. A factory reset clears the directory along with everything else under
`Apps/`; short of that, whatever you write stays until some app deletes it.

For a few kilobytes of calibration that costs nothing. For anything large — a downloaded map
set, say — it means leaving data on the watch with no owner and no way for the user to
reclaim the space. If your app writes something big, give the user a way to remove it, the
way Treadmill offers a clear-calibration action.

## In the simulator

Two differences, and the second one is the one that will catch you.

The simulator's filesystem root is `Output/`
([`Kernel.cpp:17`](../Libs/Source/Simulator/Kernel/Kernel.cpp)). It passes `..` through to
the host filesystem instead of clamping it to that root, so `../SharedData/` lands beside
`Output/` and not inside it. Apps behave correctly. The files are just not where you would
first look for them.

More importantly, the simulator does not enforce the whitelist at all. It concatenates its
prefix with your path and hands the result to the host filesystem, so `../anything/at/all`
works in the simulator and fails on the watch — and the simulator gives you no hint that you
have left the sandbox. If you invent a new shared path, check it against the rule in *The
path* above rather than against what the simulator accepts.
