# Track Run

Run for a 400 m athletics track. It is [RunLVGL](../RunLVGL), the Run app on LVGL, with its distance taken from the runner's lane on the track: it learns where the track is on the first run there, then places every GPS reading on the lane line, so the distance is right in any lane. It stores up to 10 tracks, and handles road and track in one run: a warm-up from home, the session, a cool-down home.

Everything else is Run's: laps (the Lap button and auto-lap), the lap pop-up with its delta against the lap before, the LAP PACE summary page, phone workouts, Intervals, and run alerts. Track Run adds:

- **The lane.** The runner sets it: a lane, or "No lane" for the road. On the start screen ("Track 1, Lane 1" under Start, and Lane to change it), in the pause menu, or by holding Lap for 0.8 s in a run (Settings > Lane Shortcut, on by default), which opens the lane picker without pausing. Setting a lane on the road joins the track there: a stored track if one is near, else a new track the app learns. On the road the app joins a stored track by itself when the runner arrives on it, on the lane last run there, and says so ("Track 2", "Lane 1"); leaving a track by itself goes back to the road.
- **Metres.** Auto-lap by distance is Off, 200, 400 (the default), 800, 1000 or 1600 m, on any watch. Laps are in metres on the lap pop-up and the summary.
- **The lap clock.** The first face of a run: the lap's clock, the last lap and its delta, this lap's metres and the pace, the lane and the track, and a ring round the bezel filling towards the auto-lap distance (400 m with none).
- **Stored tracks.** Settings > Tracks lists them; one can be forgotten. The summary's TRACK page shows the track, its calibration, the lanes, and road and track distance.
- **The FIT file.** Sub-sport Track; positions on the lane line on a calibrated track; the elevation held on a track, with no climb counted there.

An auto-lap is the runner's own distance. In lane 1 a 400 m auto-lap ends at the same place each lap; in an outer lane, which is longer, it ends a little further round each time. The Lap button gives line-to-line times in any lane.

The live pace is the run's distance over a rolling window of about 12 s, not the GPS speed.

## Where things are

| Where | What |
|---|---|
| `Software/Libs/Header/Oval/`, `Sources/Oval/` | The track logic: counting mode, calibration, stored tracks, the runner's lane, road and track segments. Plain C++, no SDK. See [docs/OVAL.md](docs/OVAL.md). |
| `Software/Libs/Header/Run/`, `Sources/Run/` | Between the track logic and the service, no SDK: `TrackSource` (the run's distance, live speed and altitude), the pace window, the sensor clock, the GPS trace's lines. |
| `Software/Libs/Sources/Service.cpp` | RunLVGL's service, with its distance and speed from `Run::TrackSource`, and the lane and stored-track messages. |
| `Software/Apps/LVGL-GUI/gui/` | RunLVGL's screens, with Track Run's: the lane picker, the lap clock face, the track notice, Settings > Tracks and the summary's TRACK page. |
| `Software/Apps/TrackRun-CMake/` | The watch build. |
| `Tests/Host/` | Host and service tests (GoogleTest). |
| `docs/` | [OVAL.md](docs/OVAL.md) (the track logic) and [GPS_TRACE_FORMAT.md](docs/GPS_TRACE_FORMAT.md) (the GPS trace). |

In RunLVGL, "Track" already means "the activity" (`TrackScreen`, `Track::Data`). So code about the athletics track is **Oval** (`Oval::Session`, `Oval::Tracker`, `Oval::TrackStore`).

## Building

As for every example app ([Docs/sdk-setup.md](../../../Docs/sdk-setup.md)): `UNA_SDK` set, and cmake, ninja and the Arm GNU tools on the PATH. The LVGL submodule must be checked out (`git submodule update --init` in the SDK).

```
cmake -S Software/Apps/TrackRun-CMake -B build -G Ninja
cmake --build build
```

A tester build (`-DTRACKRUN_BUILD=tester`) also writes a GPS trace beside each activity ([format](docs/GPS_TRACE_FORMAT.md)); the default, `release`, leaves the writer out. The simulator writes the trace too.

## Host tests

The SDK's host tests build these too, so `ctest` in the SDK's `Tests/Host` build runs them. On their own:

```
cmake -S Tests/Host -B Tests/Host/build
cmake --build Tests/Host/build --config Release
ctest --test-dir Tests/Host/build -C Release --output-on-failure
```

- **`trackrun-host-tests`:** the track logic and `Run::TrackSource` on synthetic GPS from `TraceGen`. Random numbers and the generator's maths are portable (`PortableRandom.hpp`, `PortableMath.hpp`), so every compiler gives the same traces.
- **`trackrun-service-tests`:** the lane settings, the summary's track fields, the stored tracks file and the GPS trace, on the SDK's in-memory file system.
- **The golden file:** `Hdop.NoHdopAtAllMatchesGolden` checks events exactly, metres to 1 mm and times to 1 ms against `Tests/Host/data/hdop_unknown_golden.txt`.
- **`Targets`:** goals the track logic does not meet yet, with their thresholds kept. They are left out of the default run: `TRACKRUN_TARGETS=1 ctest ... -L targets -V`, or `--gtest_filter=Targets.*`.
- **Seed sets:** `TRACKRUN_SEED_SET=1`, `2` or `3` moves every randomised test onto other draws (0 is the default).
- **Sweeps:** `TRACKRUN_SWEEP_RUNS=5000` (counting mode and Tracker), `TRACKRUN_SESSION_RUNS=5000` (mixed road and track runs) and `TRACKRUN_NEIGHBOUR_RUNS` run the sweeps at that size.

**Changing the track logic.** Some single-scenario tests have limits set on one draw. A change to start-point, lap or segment logic needs the default run on seed sets 0 to 3 **and** the 5,000-run sweeps on all four, compared with a "before" binary built from the previous commit on the same test tree. A default run alone can pass a change that breaks other draws.

A GPS trace from the watch or the simulator replays through the host tests: see [GPS_TRACE_FORMAT.md](docs/GPS_TRACE_FORMAT.md).

## The simulator

```
cmake -S Software/Apps/LVGL-GUI/simulator -B Software/Apps/LVGL-GUI/simulator/build -G "Visual Studio 18 2026" -A Win32
cmake --build Software/Apps/LVGL-GUI/simulator/build --config Debug
```

(Linux: `-G Ninja -DCMAKE_BUILD_TYPE=Debug`, with `libsdl2-dev`.) `TrackRunSimulator` is in `build/bin`. It takes the SDK simulator's `--script`, `--headless`, `--out` and `--fs-root` options and its `gps replay` command ([Docs/Simulator.md](../../../Docs/Simulator.md)); `simulator/smoke.txt` is the CI run. A replay of laps of a 400 m oval shows the track features: the first run on it learns the track (four laps to lock), the next finds it stored.

## Known limits

The track logic's limits, with their figures from the synthetic tests, are in [docs/OVAL.md](docs/OVAL.md). Also:

- A track lap passed while paused (walking on round the track) is credited after the resume, as the track logic counts laps by where the runner is on the track.
- GPS stays on in the menus, so the start screen can show the track.
