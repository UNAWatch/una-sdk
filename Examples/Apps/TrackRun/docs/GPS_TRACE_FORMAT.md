# Track Run: GPS trace format

Track Run can write a GPS trace (`trace_*.csv`) beside a run's FIT file, so a run can be studied and replayed on a PC: one row per GPS reading, with what the app made of it.

It is written in tester builds (`-DTRACKRUN_BUILD=tester`) and by the simulator. Release builds leave the writer out. See [README.md](../README.md).

On the watch the files are in `Apps\TrackRun\Activity\YYYYMM\`, named with the FIT file's stamp:

```
activity_20261028T122800.fit   the run
trace_20261028T122800.csv      the GPS trace
```

`Apps\TrackRun\tracks.txt` holds the stored tracks. The GPS trace carries the ones the run started with (the `store` meta lines), and `tracks.txt` shows what the run made of them.

## The GPS trace

- Watch writer: `Software/Libs/Sources/GpsTraceWriter.cpp`, with lines from `Run::TraceFormat` (integers in fixed decimals, so the watch needs no float `printf`).
- Host reader and writer: `Tests/Host/TraceCsv.hpp` and `.cpp`. The writer writes version 2; the reader reads versions 1 and 2.
- Examples: `Tests/Host/data/trace_fixture_v2.csv`, and `trace_fixture_v1.csv`, kept to check that version 1 files still load. Both come from the host tests' trace generator.

### Layout

The file has four parts, in this order:

1. **Version line.** Always the first line: `# trackrun-gps-trace v2`. A reader rejects a file without it, or with a version newer than it knows.
2. **Meta lines**, optional. One `# key=value` per line.
3. **Column header.** The column names, comma-separated.
4. **Data rows.** One per GPS reading, oldest first. Every reading in each batch the sensor delivers is written.

Rules for readers:

- Columns are found **by name**, so their order does not matter, and unknown columns are ignored. Only `t_ms`, `fix`, `lat` and `lon` are required.
- A **blank field** means "not available". Each column below says what that stands for.
- **Lines starting with `#`** after the header are comments (or action lines, below), and are skipped by the row reader.
- Blank lines are skipped. Line endings may be `\n` or `\r\n`. Numbers use a full stop as the decimal mark.

### Input columns (what the sensors gave)

| Column | Type and format | Meaning | Blank means |
|---|---|---|---|
| `t_ms` | unsigned integer | The reading's sensor timestamp in ms (`SensorDataParserGpsLocation::getTimestamp()`). Wraps after 2^32 ms, about 49.7 days. | Not allowed |
| `fix` | `1` or `0` | Coordinates valid (`isCoordinatesValid()`). With no fix, `lat` and `lon` hold whatever the parser gave, and are not used. | Not allowed |
| `lat` | degrees, 7 decimals | Latitude as the sensor gave it, a 32-bit float printed in full. | 0 |
| `lon` | degrees, 7 decimals | Longitude, likewise. | 0 |
| `alt_m` | metres, 1 decimal | GPS altitude. Only calibration uses it, for the track's mean altitude. | Unknown |
| `hdop` | number, 2 decimals | The parser's precision figure (`getPrecision()`), unconverted. Track Run reads it as HDOP, a unitless measure of the satellite geometry (about 1 good, 2 or more poor), and turns it into metres with `Oval::Config::uereM` (see [OVAL.md](OVAL.md)). | Unknown (read as -1) |
| `accuracy_m` | metres, 1 decimal | Version 1 only: the same figure, read as metres (below). | Unknown (read as -1) |
| `gps_dist_m` | metres, 2 decimals | The kernel's running GPS distance (`GPS_DISTANCE`), the latest value at the time of this reading. That sensor arrives separately from `GPS_LOCATION`, so it can be up to a second older than the position. | Not available yet (before Start, or no sample) |

### Output columns (what the app made of it)

| Column | Type and format | Meaning | Blank means |
|---|---|---|---|
| `track_dist_m` | metres, 2 decimals | The run's total after this reading, road plus track (`Session::totalDistanceM()`). | Not running (pre-run rows) |
| `lap` | integer | Laps of the track in the whole run after this reading (`Session::runLaps()`). These are the track logic's laps, not the run's (the Lap button and auto-lap). | Not running |
| `mode` | word | `pre` for the readings primed before Start, `count` for counting mode (on the road too), `cal` when the current track is measured in calibrated mode, `pause` while the run is paused. | Treated as `count` |
| `lane` | integer | The Session's lane after this reading: the runner's, or the lane a track joined by itself takes. Pre-run rows: the lane Start will use. | "No lane" (the road) |
| `segment` | word | `road` or `track`: the Session's segment after this reading. | Pre-run rows |

### Meta lines

| Key | Meaning |
|---|---|
| `start_ms` | Sensor time of the Start press, on the `t_ms` clock. |
| `lane` | Lane at Start; 0 is "No lane". |
| `table` | `outdoor`, the only value. A reader may take a missing key as outdoor. |
| `app_version` | The build that wrote the file. |
| `store` | One line per stored track the run started with, as `tracks.txt` holds it (`track v=1 n=1 lat=... crc=...`), so a replay can rebuild the store. May appear several times. |
| `hdop_scale` | Simulator only. The simulator's precision figure is not HDOP-like; the service multiplies it by this (1 / `uereM`, 0.25) before the track logic, and a replay does the same. |
| `hdop_note` | Free text saying the above. |
| `source` | Free text about where the file came from (the trace generator writes it). |

Readers ignore keys they do not know.

### Action lines

After the column header, the watch writes the runner's actions as `#` lines at the sensor time they happened, so a replay can repeat them:

| Line | Meaning |
|---|---|
| `# lane_set=<t_ms>,<lane>` | A lane set during the run, from the pause menu or a hold of Lap: the lane, or 0 for "No lane" |

A lane set also shows in the `lane` column, which changes as well when the run joins or leaves a track by itself.

### Example

```
# trackrun-gps-trace v2
# start_ms=1005500
# lane=2
# source=synthetic: TraceGen seed 2026, lane 2, 3 laps at 5 m/s, 3 m noise
t_ms,fix,lat,lon,alt_m,hdop,gps_dist_m,track_dist_m,lap,mode,lane,segment
1004000,1,55.9496536,-3.1899691,50.1,0.80,,,,pre,2,
1005000,1,55.9496880,-3.1899846,49.6,0.80,,,,pre,2,
1006000,1,55.9496803,-3.1899347,50.5,0.80,1234.50,0.00,0,count,2,track
...
1262000,1,55.9495773,-3.1894846,50.2,0.80,2561.00,1258.26,3,count,2,track
```

`track_dist_m` stays at 0 for the first few seconds, until the runner is 20 m from the start point ([OVAL.md](OVAL.md), counting mode).

### Size and flushing

A row is about 65 to 72 bytes, so an hour is about 250 KB. The file is flushed every 10 s, so a crash loses at most that much of the tail.

### Replaying a trace

`TraceCsv::toTrace()` turns a file into the calls the service makes, and `Tests/Host/Replay.cpp` runs them through `Oval::Session`:

- **Pre-run.** Rows with mode `pre` are passed to `prime()`.
- **Start.** `start()` is called at `start_ms`. Without it, Start is placed halfway between the last `pre` row and the next row.
- **Lane.** The starting lane comes from the `lane` meta line, or else from the first row after Start. A change in the `lane` column is a `setLane()` before that row; a blank is `setLane(0)`, "No lane". A file with no `lane` column sets no lane. This suits a file made by `fromTrace()`, whose lanes are all the runner's.
- **Pause and resume.** These are placed halfway between the rows where `mode` turns to `pause` and back. So a replayed lap that holds a pause can differ from the watch's by up to half a second of active time. Lap counts and credited distances are not affected.
- **Readings.** Every row from Start on goes to `update()`, and `finish()` is called after the last.
- **Actions.** `toTrace()` reads no action line. `TraceReplay.WatchTrace` (below) replays a watch's file from them: it drops the lane changes `toTrace()` took from the `lane` column, which also follows the joins and exits the run made by itself, and adds the `lane_set` lines as `setLane()`, "No lane" included.

`TraceReplay.WatchTrace` in `Tests/Host/WatchTrace_test.cpp` replays one file and compares the laps and total with the file's last row:

```
$env:TRACKRUN_TRACE = "path/to/trace_20261028T122800.csv"
Tests/Host/build/Release/trackrun-host-tests.exe --gtest_filter=TraceReplay.WatchTrace
Remove-Item Env:TRACKRUN_TRACE
```

A trace that replays to the watch's result can be copied into `Tests/Host/data/` and made into a test with `TraceCsv::read` and `TraceCsv::toTrace`, asserting what the run should have given.

### Versions

Raise the version number only for a change an old reader would misread. Adding a column or a meta key does not need one.

Version 1 has the precision figure in an `accuracy_m` column, in metres. Version 2 has it in an `hdop` column, as HDOP.

- **Reading a version 1 file.** `accuracy_m` is divided by `Config::uereM` (4 m) to give HDOP. A figure of 3.7 is HDOP 0.925, an estimated error of 3.7 m again, so a replay of `trace_fixture_v1.csv` is unchanged.
- **In a version 2 file** an `accuracy_m` column is ignored, like any column the reader does not know.
