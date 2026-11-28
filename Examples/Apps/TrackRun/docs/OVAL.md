# Track Run: the track logic (Oval)

This is a reference to the code that measures a run on a 400 m athletics track: lap counting, fitting the track to the GPS trace, stored tracks, the runner's lane, and runs that mix road and track. It describes what the code does and why, with the figures from the synthetic tests that set its thresholds. Build, test and memory-budget instructions are in [README.md](../README.md). The GPS trace format is in [GPS_TRACE_FORMAT.md](GPS_TRACE_FORMAT.md).

## 1. What it is and where it lives

The track logic is plain C++17. It has no SDK headers, no logging, no exceptions, no RTTI and no heap, so the same sources build for the watch and for the host tests. All its state is fixed-size. It works in `float` metres; `double` is used only where precision needs it (the latitude and longitude origin subtraction, the primed start-point average, and the running sum of credited laps, exact to 0.01 m over a long run). The service includes the headers as `"Oval/Session.hpp"` and so on; `libs.cmake` globs `Sources/` recursively, so the watch build compiles them all.

The code is in the `Oval` namespace. (In the app, "Track" already means the activity, so code about the athletics track is called Oval.)

| Class | File (`Software/Libs/Header/Oval/`, `Sources/Oval/`) | Role |
|---|---|---|
| `Sample` | `Sample.hpp` | One GPS reading, and wrap-safe helpers for the millisecond clock |
| `Config` | `Config.hpp` | Every tuning threshold with its default, and the HDOP helpers (section 12) |
| `LaneTable` | `LaneTable.*` | Lap length and line offset of every lane (section 3) |
| `LocalFrame`, `Vec2` | `LocalFrame.*` | Latitude and longitude to metres east and north of an origin (section 4) |
| `LapCounter` | `LapCounter.*` | Counting mode: a virtual gate at the start point (section 5) |
| `TrackModel` | `TrackModel.*` | A fitted track, and projection onto a lane line (section 6) |
| `Calibrator` | `Calibrator.*` | Fits the track template to the GPS trace, and judges the fit (section 6) |
| `LapTiming` | `LapTiming.*` | The last few seconds of readings, and line-fitted crossing times (section 6.5) |
| `Tracker` | `Tracker.*` | One visit to one track: counting mode, calibration and calibrated mode together; stored-track alignment |
| `TrackStore` | `TrackStore.*` | Up to 10 stored tracks and their text file (section 7) |
| `Session` | `Session.*` | Road and track in one run. The one class the service uses. It owns a `Tracker` (section 10) |

The layers nest: a `Session` runs a `Tracker` for each visit to a track and counts the road between; a `Tracker` runs a `LapCounter` on every reading, a `Calibrator` in the background, and its own calibrated mode once it has a model.

## 2. Inputs and outputs

### The reading: `Oval::Sample`

| Field | Meaning |
|---|---|
| `tMs` | Sensor timestamp in ms (`SensorDataParserGpsLocation::getTimestamp()`, carried on modulo 2^32 across the watch clock's wrap by `Run::TimeUnwrap`). It wraps after 49.7 days. All arithmetic on it is wrap-safe, as signed 32-bit differences (`msDiff`, `msAfter`). |
| `lat`, `lon` | Degrees. `double` in the struct, although the sensor gives `float`. |
| `hdop` | The parser's precision figure (`getPrecision()`). It is HDOP, unitless, not metres (section 9). Negative means unknown. |
| `fix` | Coordinates valid. |
| `gpsDistM` | The kernel's running `GPS_DISTANCE` total, or NaN. Only its differences are used. |
| `altM` | GPS altitude in metres, or NaN. Only calibration uses it, for the track's mean altitude. |

A reading is **good** when it has a fix and its HDOP is at most `maxHdop` (4.0), or unknown. Only good readings make position decisions.

### The calls the service makes (`Session`)

| When | Call |
|---|---|
| Start screen, each reading | `prime(sample)`; `primedPosition(lat, lon)` to look up a stored track |
| Start | `start(tMs, lane, table, &store)`. The store must outlive the run: it is also searched for tracks met on the way. |
| Each `GPS_LOCATION` reading, every reading in a batch | `events = update(sample)` |
| Pause menu | `pause(tMs)`, `resume(tMs)` |
| The lane (pause menu, or a hold of Lap) | `events = setLane(lane, tMs, &ok)`: a lane, or `kNoLane` (0) for the road (section 8) |
| Lap button | `events = lapPress(tMs)` |
| Stop | `events = finish()`, then `storeOutcome(number)` or `storeOutcome(i, number)` for each of `visitedCount()`, each passed to `TrackStore::apply()` |
| Settings | `setRoadSplitM(m)` before Start (1,000 m, or a mile on an imperial watch) |

`Tracker` and `LapCounter` take the same calls (`prime`, `start`, `update`, `pause`, `resume`, `setLane`, `finish`, and `restartLap` for the Lap button), and can be used on their own; the host tests do.

### Events

`Session::update()`, `setLane()`, `lapPress()` and `finish()` return up to 16 events, oldest first:

| Type | Meaning |
|---|---|
| `TrackLap` | A Tracker `Lap`, `MissedLap` or `LapTimeRefined`, in `track`. `runLap` is its number in the whole run. |
| `TrackEvent` | Any other Tracker event (calibration, mode), in `track`. |
| `TrackJoined` | A known track noticed on the road and joined (section 10.3): `trackNumber` (0 if not stored) and `lane`, the lane it was joined on. A lane the runner sets raises none. |
| `SegmentChanged` | `from` and `segment`, at `tMs`. The time can be in the past: where the runner came onto the track or left it. |
| `RoadKilometre` | Road distance passed another whole `roadSplitM` in the road segment: `kilometre` (1-based, whatever the split length) at `tMs`, interpolated on the GPS distance. |

A Tracker event (`Tracker::Event`, up to 8 an update) carries its type, lap number, time, active duration, credited length, credited total and mode. Its types are `Lap` (ended on a crossing), `MissedLap` (credited without a seen crossing), `LapTimeRefined` (a better end time for an earlier lap), `CalibrationProvisional`, `CalibrationLocked`, `CalibrationRejected` and `ModeChanged`. Lap durations are active time: pauses are excluded.

### Getters (`Session`)

| Getter | Returns |
|---|---|
| `segment()` | None (before Start), Road or Track |
| `totalDistanceM()` | Road plus track. Never decreases. |
| `roadDistanceM()`, `trackDistanceM()` | The two parts |
| `segmentLaps()`, `runLaps()` | Laps since the join, and in the run |
| `roadSplitShare()`, `roadSplitsDone()` | On the road: the share of the current split run, and splits done |
| `trackNumber()`, `lane()` | The stored track in use (0 if not stored), the lane (`kNoLane` on the road) |
| `holdElevation()` | On a track with a model: hold its altitude (the barometer applies on the road) |
| `lanePositions()`, `lanePosition(lat, lon)` | On a track in calibrated mode: the latest reading placed on the runner's lane line, for the FIT records |
| `timeOfDistance(totalM, tMs)` | On a track: when the total reached a figure, for rep endings and distance splits mid-lap |
| `segmentStartTotalM()` | The run's total where the current segment began |
| `earliestRetimeTMs()` | No event raised from now on is timed before this (section 10.7) |
| `autoJoins()` | Known tracks the run joined by itself so far (the GUI's notice) |
| `joinPoint(lat, lon)`, `joinTMs()` | The latest join |
| `largestHoldM()` | The most the shown total has held above the measured one |
| `tracker()` | The Tracker, for its own getters: `mode()`, `calibrationStage()`, `lapProgressM()`, `lapElapsedMs()`, `lastLapDurationMs()`, and so on |

## 3. Lane tables

Lanes 1 to 8:

- **Lines.** Lane 1 is measured 0.30 m out from the kerb. Every other lane is measured 0.20 m out from its inner line, and lanes are 1.22 m wide, so lane n's line is `(n - 1) x 1.22 + 0.20` m from the kerb for n of 2 or more.
- **Lengths.** Only the bends differ between lanes, so lane n is `400 + 2 pi (offset n - 0.30)` m whatever the bend radius: 400.00, 407.04, 414.70, 422.37, 430.03, 437.70, 445.37 and 453.03 m.
- `lineOffsetM(lane)` gives each line's offset, which calibration and calibrated mode use.

## 4. Local frame

- **Projection.** Equirectangular round an origin, using the cosine of the origin's latitude and the mean Earth radius of 6,371,008.8 m. The origin subtraction is in `double`; the result is `float` metres, x east and y north.
- **Accuracy.** Against haversine over 0 to 1 km on every bearing, the worst error is 0.0045% at 56 degrees north and 0.00001% at the equator.
- **Edges.** It handles the 180 degree meridian.
- **Sensor resolution.** The sensor's `float` coordinates resolve to about 0.4 m north-south at 56 degrees north.

## 5. Counting mode (`LapCounter`)

Counting mode needs no model of the track. A virtual gate sits at the start point; each forward crossing adds one lap of the runner's lane. Between crossings the part-lap is estimated from GPS distance. Whole laps are exact; only the part-lap is approximate, and that error is wiped out at each crossing. It runs on every reading in every mode, so it is always ready as a fallback.

### 5.1 Start point

- **Priming.** `prime()` keeps the last 8 good fixes. At `start()`, those from the last `primeWindowMs` (5 s) are averaged, weighted by 1 / `hdopErrorM()` squared, and the mean becomes the start point and the origin of the local frame. With no primed fixes, the first good fix after Start is used.
- **A stale start.** The first readings after Start are averaged while each stays within `gateResetM` (10 m) of their mean. If that mean is more than `staleStartM` (15 m) from the primed start point after at least 3 readings, the primed fixes were stale and the start point moves to the mean. The check ends once the runner leaves, after 10 readings, or once the mean has moved 5 m (half of `gateResetM`) from where the first three put it: otherwise a slow jog off would drag the mean, and the start point, along with the runner.
- **Judging from the first readings.** When the runner leaves before three readings agree, or the first three already move, and there were at least 3 primed fixes, the start is judged from the first readings instead (`judgeFromFirst`). The first four readings, on a weighted line against time, give where the runner was at the press. They are split into halves, each carried back to the press. The start point moves to their mean when both halves lie beyond `max(staleStartM, 6 x the GPS noise at Start)` (`kStaleNoise`) and agree within 6 m (`kPressAgreeM`). A borderline case is judged again at six readings, with one stray reading left out. At six readings a start point that lies to one side of the runner's line by `max(10 m, 3 x the noise)` (`kSideFrac`, `kSideNoise`), on a line 4 standard errors clear (`kSideSigma`), moves across onto the line rather than along it: a start point 10 m beside the line turns a gate formed at 20 m by about 30 degrees. While the judgement waits, the gate sees no readings; afterwards it is given the ones it missed.
- **A rolling start.** At Start, a weighted line through the primed fixes (all of them, over up to twice the prime window) gives their velocity, its standard error and their scatter, which is taken as the GPS noise at Start. The run is a rolling-start candidate when the speed is at least 1.5 m/s and 3 standard errors (`kRollMinMps`, `kRollSigma`), from 3 primed fixes or more. It is a rolling start if the first reading after Start, carried back to the press at that velocity, lies ahead of the primed mean by at least half of what steady running gives (`kRollLeadFrac`). The start point is then that reading carried back: where the runner was at the press, not 2.5 to 3 s of running behind it. At the fourth reading it is refined from the two halves of the first four readings, as above. A walker under 1.5 m/s keeps the primed mean.

### 5.2 The gate

The gate is a line through the start point. Its direction comes from the runner's first move away:

1. **Gate line.** When `gateDirConfirm` (2) good readings in a row are `gateDirDistM` (20 m) or more from the start point, the direction from the start point to their mean sets the gate line. A reading back within `gateResetM` (10 m) forgets it, and the next move away sets it again.
2. **Lap 1 starts counting** from that moment: progress is the straight-line distance there (about 20 m) plus scaled GPS distance after it. GPS distance from standing or jogging about at the line beforehand is dropped. Until then the part-lap is 0.
3. **Gate final.** The gate becomes final at the first crossing of its line on the **far side of the track**, at least `armFarM` (40 m) from the start point. Going round, the runner crosses the line there heading back, which fixes which way the gate faces.

Fixing the direction from the first move alone fails: jogging back and forth across the line, or a run-up from behind it, can point the gate backwards, after which no lap counts. Jogging along the track never crosses the gate line 40 m from the start point, so neither can set it.

### 5.3 Arming

A forward crossing near the start point counts only when the gate is **armed**: the runner has crossed the gate line backwards on the far side of the track since the last lap. Every real lap does that once. Standing, jogging or resting on the line never does, so they never count a lap; the distance and time guards alone would let a 90 s rest at the line count one. On a bend start, the far-side crossing is on the same bend, about 60 to 75 m before the gate rather than half a lap round, which is why `armFarM` is 40 m, not 50.

### 5.4 Crossings and lap timing

A **crossing** is a change of sign, negative to positive, of the position along the gate direction between two consecutive good readings. Its time is interpolated linearly between the two readings' `tMs`, so lap times carry fractions of a second and never come from a 1 Hz tick. It counts at once when the gate is armed, the crossing point (on the line between the two readings) is within `gateRadiusM` (25 m) of the start point, the readings are at most `maxCrossGapMs` (10 s) apart, the run was not paused at the crossing, and the guards pass.

### 5.5 Guards

- **Distance.** At least `minLapFrac` (0.70) of a lap since the last lap, by scaled GPS distance.
- **Time.** At least `minLapMs` (40 s) of active time since the last lap.
- **Reading quality.** Readings over `maxHdop` make no position decision: start point, gate, crossings, or the step-sum fallback.
- **Speed.** GPS distance faster than `maxSpeedMps` (12 m/s) is cut to 12 m/s, measured from the last time the kernel total moved, so the real jump after a dropout is kept.
- **Arming** (section 5.3).

### 5.6 Missed laps

A lap can go uncounted at the moment of crossing. Three rules credit it, each as a `MissedLap`:

1. **Pending pass.** A forward crossing within `passRadiusM` (35 m) that cannot be counted at once (crossing point 25 to 35 m out, a gap over 10 s, or a pause) becomes a pending pass. It is credited when the scaled GPS distance since the last lap reaches `missedLapFrac` (1.5) laps, or sooner at the next far-side crossing, timed at the pass. A countable crossing soon after (under 0.7 of a lap on: the same passage of the line) replaces it as a normal `Lap`. Walking over the line while paused is handled this way: the lap is credited after resuming, timed at the pause.
2. **Hidden by a gap.** A real crossing that ends a lap 1.5 or more laps long by distance, in a lap with a gap of over 10 s in good readings, first credits the lap the gap hid, ending where GPS distance said one lap. The crossing then ends the next lap.
3. **Distance alone.** With no pending pass, a lap is credited at `missedLapNoPassFrac` (2.5) laps of scaled GPS distance, at the one-lap point, but only if the lap had a reading gap. With readings every few seconds a pass is always seen, and GPS distance that piles up while the runner rests on the line must never credit a lap.

A plain "credit at 1.5 laps of distance" rule fails on a detour: a 200 m detour adds half a lap of GPS distance, so 1.5 laps arrives just before the real crossing, the lap is credited early, and the real crossing is then blocked. So distance credits a lap only with evidence that the runner passed the gate (rule 1), or after a gap (rules 2 and 3). After a credit the gate stays armed. At Stop, `finish()` credits a pass still pending.

### 5.7 Part-lap estimate and total

- **Estimate.** GPS distance since the last lap times `ratio`, capped at `partLapCap` (98%) of the lap.
- **Ratio.** The credited length of the last lap that ended on a real crossing, divided by its GPS distance: true metres per GPS metre. It starts at 1.0, is clamped to 0.80 to 1.25, is never taught by a credited lap, and carries over a lane change.
- **No GPS distance.** If `gpsDistM` is NaN, GPS distance is the sum of steps between good positions. A NaN drops the reference, so a kernel total that grew meanwhile is not counted twice when it returns.
- **Pauses.** GPS distance covered while paused is not counted.
- **Lane change.** `setLane()` mid-lap closes a segment: the lap is credited as (fraction at the change x old length) + (the rest x new length). Progress is held as a lap fraction, piecewise, so the estimate does not jump.
- **Total.** Credited laps plus the estimate. It never decreases: at a crossing the lap adds its full length while the estimate (at most 98%) restarts near zero; through a lane change it is continuous; if the runner jogs out and back before setting off, what was shown for lap 1 is kept as a floor.

`lapFraction()` gives the uncapped fraction: over 1 when a pass of the line has not been seen. The Tracker uses it (section 6.3).

With GPS distance about 12% high, the part-lap error averages 15 to 24 m in lap 1, before any ratio is learnt, and 6 to 9 m from lap 2. White jitter alone makes GPS distance wander by about plus or minus 19 m a lap, which no ratio can remove.

### 5.8 Pause

Readings keep arriving while paused. No lap counts and the gate is not set up then; a pass of the line becomes pending. Paused time is excluded from lap durations and elapsed time. A crossing between a paused reading and an active one counts only if its interpolated time is after the resume.

## 6. Calibration and calibrated mode

The Tracker also fits the track's shape and position to the GPS trace, then places readings on the runner's lane line. This removes most of the part-lap error and times laps better.

### 6.1 The track model (`TrackModel`)

- **Shape.** Two straights and two semicircular bends. The kerb radius R may be 35 to 38 m, and each straight is `(400 - 2 pi (R + 0.30)) / 2` m, so lane 1 is always 400 m. A lane's line is the same shape at radius R plus its offset, so every lane's lap matches the lane table to under 0.01 m for any R.
- **What it holds.** Centre (local x, y), the heading of the straights in [0, 180) degrees, R, the running direction, the mean altitude of the fixes used, and the centre's latitude and longitude. `placeIn(frame)` puts a stored model into a new run's frame.
- **Stations.** A station `tau` in [0, 4) names a place the same way in every lane: 0 to 1 the first straight, 1 to 2 the first bend, and so on, in the running direction. The start line, and where a lane change happened, are stations.
- **Projection.** `project(p, lineOffset)` gives the distance along the line from station 0, the signed lateral offset (positive outward) and the station.

### 6.2 Calibration (`Calibrator`)

**Collecting.** Good fixes, in the counting-mode frame, are stored when the runner is `calSpacingM` (3 m) from the last stored point. A point is 6 bytes: decimetres as `int16`, the lane the runner had set, and its HDOP in twentieths. The buffer holds 512; when it fills, every other point is dropped and the spacing doubles, so it stays spread over the whole run. Points are not collected while paused, or while the runner is off the track.

**Coverage.** Every 8 points, the angle the points sweep round their centroid is summed. A closed loop adds a full turn whatever its shape; jogging back and forth cancels. A lap of coverage is a full turn with all of 36 angle bins visited. Its sign gives the running direction.

**The fit.** Each time coverage completes another lap, the template is fitted. Unknowns: centre x and y, heading, R, and the runner's mean distance outside their lane line.

- **Residual.** A point's signed distance from the kerb outline, less the line offset of the lane it was tagged with, less the runner's offset. Tagging each point with its lane means a lane change set on the watch does not bias the fit.
- **Start.** The centroid and main axis of the points (the eigenvector of their 2 x 2 covariance), with R = 36.5 m; from the second fit on, the previous fit.
- **Solver.** Levenberg-Marquardt on the 5 unknowns with analytic derivatives, inside iteratively reweighted least squares with Huber weights (k = 1.345 times a robust scale: 1.4826 times the median absolute residual, from a histogram so no residual buffer is needed), each times the point's `hdopWeight()`. The 5 x 5 normal equations are solved by Cholesky on the stack. At most 60 iterations.
- **Why.** Levenberg-Marquardt is stable from a rough start where plain Gauss-Newton can overshoot on the heading. Huber weights keep a lane change, a bad patch or a few wild readings from dragging the fit, but stay convex enough to converge from the centroid. A harder weight (Tukey) can lock onto a wrong minimum.
- **Noise on the bends.** A noisy point round a bend lands outside the line on average, by about variance / (2 x radius): 0.34 m at 5 m noise. Uncorrected, it biased R by up to -0.18 m at 4 to 5 m noise. The fit removes it using the robust scale; the mean signed R error over 200 runs is 0.01 to 0.02 m.
- **What R rests on.** The runner's offset is free, so the template can take any stadium shape: R is set by where the straights end, not by the 400 m. A 300 m track fits perfectly with R = 52.4 m and the runner 16 m inside their line. R is therefore the noisiest of the four, and the radius check is what rejects most non-standard tracks.

**Stages.** Provisional after `calProvisionalLaps` (1) lap of coverage, if the fit passes the quality check; refined at each further lap; locked from `calLockLaps` (lap 4) once the centre has moved at most `lockCentreM` (1.5 m) and the heading at most `lockHeadingDeg` (1.5 degrees) since the previous fit. Over 400 test runs at 2 to 5 m noise the moves at lap 4 were at most 1.22 m and 1.10 degrees, so every run locked at lap 4. A fit not locked after `calMaxLaps` (6) is rejected as disagreeing.

**Quality check.** A fit fails, checked in this order, when:

- **Coverage.** A twelfth of the fitted track has fewer than `minBinPoints` (2) points within 3 robust scales of it.
- **Radius at the edge.** R ends within 0.05 m of the edge of the search range, `fitRadiusMinM` to `fitRadiusMaxM` (32.5 to 39.75 m). The range is wider than 35 to 38 m so a real 35 m or 38 m track is not cut off by noise.
- **Residual.** The robust scale is over `maxScaleM` (9 m).
- **Shape.** The noise-corrected systematic residual is over `maxShapeM` (2.0 m). Residuals are averaged in 24 station bins; each bin's mean, less what noise alone gives it, shows where the track is the wrong shape.

A failed check before `calRejectLaps` (lap 3) waits for more data. From lap 3 it rejects the track as not standard. At lap 2, standard tracks at 5 m noise and the double-bend shape below came within about 0.5 m of each other in R; from lap 3 they are clear.

**Margins** (from lap 3, with R fitted on an opened range to see where it lands):

| | Standard tracks (R 35 and 38 m, 2 to 5 m noise, lanes 1 to 8) | Non-standard shapes |
|---|---|---|
| R, top edge 39.70 m | at most 39.24 m: 0.46 m inside | double bend 1.60 m beyond; park loop 4.16 m; 300 m track 11.76 m; circle 15.30 m |
| R, bottom edge 32.55 m | at least 33.47 m: 0.92 m inside | none come near it |
| Shape, limit 2.0 m | at most 0.78 m | park loop 0.93 m beyond; circle 2.12 m beyond |
| Scale, limit 9 m | at most 5.40 m | circle well beyond |

All 80 non-standard runs (four shapes, noise 0 to 5 m) and all 600 standard runs were decided correctly. The shapes are a 300 m track, an elongated lumpy 400 m loop, a 400 m circle, and a double-bend track with 60-degree arcs at 51.543, 34.000 and 51.543 m. The double bend is only caught by R, by 1.6 m.

### 6.3 Calibrated mode

**Position.** Each good reading, less the tracked GPS offset (drift, below), is projected onto the current lane's line. Its position is kept as metres from the start line, unwrapped (the current lap runs from 0 to the lap length).

**Smoothing.** An alpha-beta filter on that position, `abAlpha` 0.5 and `abBeta` 0.12. At a steady speed it has no lag; from standing to 5 m/s it trails by at most 3.5 m, and 1 m after 7 s; stopping dead, it overshoots by up to 3.5 m. Lower constants smooth noise better (0.3 and 0.05 take about 0.3 m off the part-lap 95th percentile at 3 m noise) but trail by 7.6 m when starting a rep. A reading further from the prediction than `outlierM` (20 m) times `hdopWiden()`, plus 6 m/s (half `maxSpeedMps`) times the gap, is an outlier; three in a row that agree restart the filter there. The filter starts at the runner's current speed, from a line fit through the last 5 s.

**Distance.** A high-water mark on the smoothed position. Progress counts only past the furthest point reached, and only while the smoothed speed is at least `stillMps` (0.5 m/s), so noise, jogging back and forth and standing still add nothing; crediting is delayed, never lost. Each stretch counts on the line of the lane being run at the time.

**Laps.** A lap ends when the high-water mark passes the start line again. Needing a whole lap of progress is the arming. The 40 s guard still applies. A lap with no lane change, Lap press, skip or rejoin is credited exactly its lane length; otherwise the metres run on the lane lines.

**Gaps.** After a gap of over 10 s in good readings (a dropout, or a spell of HDOP over 4), the filter carries on at the average speed over the gap. A lap that ended in the gap is credited at once, as a `MissedLap` timed by interpolation across the gap.

**Pauses.** At the pause, the way run since the last reading is credited at the smoothed speed, short of the start line so that a lap still ends on a reading. Nothing counts while paused. At the first reading after the resume, the odometer's step for that reading is credited: it counts the share of the interval after the resume, as Run's distance does, capped at `maxSpeedMps` since the resume. That reading then sets where running starts again, ahead of the high-water mark or behind it: as in Run, what was run before the pause and what is run from the resume both count, and the way between does not. That reading's GPS noise can add a few metres to the lap or take a few off.

**The switch mid-run.** When the first provisional fit arrives, the switch waits until the runner is between `switchFromFrac` and `switchToFrac` (25% and 75%) of the lap from the start line, on the track and not paused, and counting mode holds no uncounted pass. It also waits while counting mode's `lapFraction()` runs half a lap or more ahead of where the model puts the runner (`kSwitchOverdueFrac`): counting mode has missed a pass (for example, a poor-HDOP spell hid the far-side crossing) and the lap is still its to credit. In normal runs the two agree within about 0.1. At the switch, laps already credited stay; the current lap is re-measured on the model from the start line, lane by lane if the lane changed earlier in the lap (up to three changes are kept for this). If that is less than shown, the shown total holds until it catches up: at most 43 m over 10,000 runs, mostly where the generated GPS distance over-read early laps.

**Refits.** A refined or locked fit keeps the runner at the same place on the track and re-measures the lap there; a drop is held on the display.

**Lane changes.** `setLane()` keeps the runner's station and moves the measurement to the new lane's line. Lane 1 to 8 at half a lap, on the back straight, credits the physical distance to within 0.7 m.

**Off the track.** Off the track is outside the lanes, not off the runner's own line: a runner on another lane than the one set (a stored track's last lane) is still on the track, and the distance is theirs to put right by setting the lane. `offTrackReadings` (3) readings in a row further than `offLanesM` (12 m) times `hdopWiden()` beyond the lanes (outside lane 1's line to the last lane's, `offLanesM()`) mean the runner has left the track. Progress freezes at once, and goes back to the last reading within 5 m (a third of `offTrackM`, 15 m) of the runner's own line that still moved along it, so the filter's momentum does not credit the turn away or the way across the other lanes. That reading counts only if it came within `kSafeMaxAgeMs` (20 s) of the reading that confirms the runner off; a runner on another lane than the one set is never that close to its line, and an older reading would date the exit back. Without one, the runner left at the first reading off. After `offTrackMs` (30 s) off, the mode drops to Counting. Back within three quarters of `offLanesM` (9 m, times `hdopWiden()`) beyond the lanes for `rejoinReadings` (5) readings, the line restarts from the first of them: nowhere is the runner both off and back on, so a runner on a path just beyond the margin leaves once. 12 m beyond the lanes is about 16 m from a middle lane's line; in the tests 10 m raised false exits under poor HDOP, and 13 m and more let a runner leaving along a path beside the track run on a lap.

- After a short spell (still calibrated), the way along the track is credited up to as far as running on at the earlier speed would have gone; only the excess is not. A false alarm costs nothing. A spell with a pause in it credits none of the way: where the runner went while paused is not credited, as for a pause on the track.
- After a real absence, used on its own a Tracker never credits the detour. If the runner passed the start line meanwhile, the lap is credited with what was run of it. Under a `Session`, which is what the service uses, a confirmed exit instead ends the track segment, and the way off is counted as road (section 10.5).

**Drift tracking.** Once the model is locked, or from the end of lining up with a stored model, heading and R stay fixed but a slowly varying 2D offset between GPS and the model is tracked. Each on-track reading's lateral offset measures that offset along the line's normal. Normal equations weighted by `hdopWeight()` over about the last lap are solved for a translation (a uniform shift of the runner's own line is the same all the way round and cancels out). The offset moves towards it by at most `driftPerLapM` (1 m) a lap of progress. With drift growing by 0.012 m/s (about 18 m) over 27 minutes after lock: no false off-track freeze, part-lap error stayed near its level after lock (95th percentile 6.1 m in the first 10 minutes, 7.0 m after), and an 80 m detour at the end was still caught.

**Fallback.** On rejection the mode is NotStandard and counting mode's laps and part-lap are shown. After calibrated laps, a counting-mode lap within 20 s (half `minLapMs`) of the last credited lap is the same passage of the line and is not credited again.

### 6.4 Modes and stages

| `Tracker::mode()` | Meaning |
|---|---|
| `Calibrating` | Counting mode measures while calibration collects points |
| `Calibrated` | Every reading is placed on the lane line of the fitted or supplied track |
| `NotStandard` | Not a standard 400 m track: counting mode measures, calibration has stopped |
| `Counting` | Counting mode measures for another reason: after leaving the track |

`calibrationStage()` is None, Provisional, Locked, Supplied (a model given at Start) or Rejected. A Tracker started on the model of a track not stored, visited earlier in the run (section 10.3), also says Supplied, although its own calibration still runs and locks in the background.

### 6.5 Lap timing and rep endings (`LapTiming`)

- **Provisional time.** The lap is reported at once: counting mode's two-reading time, or in calibrated mode the smoothed position's crossing of the start line.
- **Refined time.** Once readings `refineAfterMs` (3 s) past the crossing are in, a straight line of position against time is fitted through the readings from `refineBeforeMs` (5 s) before to 3 s after, and solved for the crossing. The position is the distance along the gate direction in counting mode, or the raw total (credited distance plus the unsmoothed position on the line) in calibrated mode. A `LapTimeRefined` event gives the lap number, the new end and the new duration; the next lap starts from the refined end, so neighbouring durations stay consistent. It comes at most 4.0 s after the crossing as first reported.
- **Rep endings.** `timeOfDistance(totalM)` finds when the total reached a figure among the last 24 readings, with the same fit in calibrated mode (interpolation in counting mode).

### 6.6 Figures

**Calibration accuracy** (200 runs, noise 2 to 5 m, random heading, location, start, lane, speed, R 35 to 38 m):

| | After 1 lap, 95th percentile | At lock, 95th percentile | At lock, worst |
|---|---|---|---|
| Centre, against the true track, with drift | 2.54 m | 3.49 m | 5.87 m |
| Centre, against where the drifting GPS trace was | 1.49 m | 0.82 m | 1.20 m |
| Centre, no drift | 1.43 m | 1.03 m | 1.47 m |
| Heading | 1.67 degrees | 0.89 degrees | 1.40 degrees |
| R, with drift | 1.22 m | 0.76 m | 0.94 m |
| R, no drift | 1.22 m | 0.71 m | 1.12 m |

An R error of 0.75 m adds about 0.3 m to the part-lap 95th percentile. Heading at lock, 95th percentile: 0.86 degrees without drift, 1.09 with it.

**Part-lap error**, |shown total - true distance| at every reading in calibrated mode, 95th percentile (the first two columns from `Tracker.PartLapAccuracyAcrossNoise`, which holds each within its limit):

| Noise | Stored model | From scratch | Sweep (mixed) |
|---|---|---|---|
| 2 m | 3.7 m | 4.3 m | 5.0 m (2 to 3 m) |
| 3 m | 5.1 m | 5.6 m | 5.7 m (3 to 4 m) |
| 4 m | 6.6 m | 6.9 m | 7.4 m (4 to 5 m) |
| 5 m | 8.1 m | 12.6 m | |

Without the random-walk drift, 3 m noise gives 4.2 to 4.8 m. What is left is drift along the track, which lateral offsets cannot see on the straights.

**Lap times**, |lap duration - true|, 200 randomised runs:

| | Mean | 95th percentile | Worst |
|---|---|---|---|
| Counting mode, provisional | 0.61 s | 1.76 s | 5.82 s |
| Counting mode, refined | 0.43 s | 1.25 s | 5.82 s |
| Calibrated mode, provisional | 0.46 s | 1.29 s | 3.06 s |
| Calibrated mode, refined | 0.38 s | 1.09 s | 2.63 s |

By noise band, refined: counting 0.31, 0.44 and 0.56 s mean (2 to 3, 3 to 4, 4 to 5 m), calibrated 0.30, 0.40 and 0.46 s. White noise moves each reading independently, so each crossing time is off by about noise / speed and a lap's error is two of those; error that drifts slowly would be shared by consecutive crossings. These figures are a ceiling.

**Rep endings** (`timeOfDistance` at 200 m and 300 m into a lap, 3 m noise, 60 each): mean 0.31 and 0.30 s, 95th percentile 0.70 and 0.68 s, worst 1.04 and 0.88 s.

## 7. Stored tracks and return visits

### 7.1 The track store (`TrackStore`)

**A record** (`TrackRecord`):

| Field | Meaning |
|---|---|
| `number` | "Track 2": 1 to 10, stable |
| `lat`, `lon` | Centre, `double` degrees, kept to 9 decimal places |
| `headingDeg`, `kerbRadiusM`, `clockwise`, `altM` | The model: heading in [0, 180), R, the direction it was last run, mean altitude (NaN if unknown) |
| `scaleM`, `shapeM` | Fit quality (the calibrator's robust scale and shape figures) |
| `firstSavedS`, `lastUsedS`, `uses` | UNIX seconds passed in by the caller (Oval reads no clock), and runs that used it |
| `fits` | Visits blended into the model: its weight in a blend |
| `lane` | The lane last run there, 0 if unknown: the lane a join noticed takes (section 10.3) |
| `version` | The format version it was written with |

**Numbers** are slots: Track n lives in slot n. Forgetting one leaves the others alone; a new track takes the lowest free number. When all 10 are in use, a new one replaces the least recently used (oldest `lastUsedS`; a tie goes to the earliest saved) and reuses its number; `saveNew()` reports which one went.

| Call | What it does |
|---|---|
| `findNear(lat, lon)` | The track whose centre is nearest, if within `storeNearM` (150 m) |
| `saveNew(model, quality, now)` | Adds a track; returns its number and the one replaced, if any |
| `update(number, model, quality, now)` | Replaces the model, keeping the number and first date; counts as a use |
| `blend(number, model, quality, now)` | A visit that agreed: stored values move towards the visit's by 1 / (weight + 1), weight = visits so far, at most `storeBlendMaxWeight` (9). Headings blend across the 0/180 wrap. |
| `touch`, `forget`, `list` | A use; remove one; all of them in number order |
| `apply(outcome, number, now)` | One call per visited track at the end of a run. Says what changed and whether the file needs saving. |

**Recognition at Start.** `primedPosition()` gives the same weighted average the start point uses. The start screen looks it up with `findNear()` ("Track 2" or "New track"); `Session::start()` does the same lookup and supplies the stored model.

**The file.** One line per track, `key=value` words, after a header line:

```
trackrun-tracks v=1 count=10
track v=1 n=1 lat=55.950000797 lon=-3.190000000 hdg=107.4271 r=37.3391 cw=1 alt=145.09 scale=2.229 shape=0.100 fits=1 uses=1 first=1760000000 last=1760000000 crc=8e79ebcd
```

- **Why this format.** It reads in any text editor, parses with no heap and no JSON library, takes keys in any order, and lets a later version add keys that this one skips. The optional key `ln=` (the lane) is written only when known. A full store is about 1.7 KB (`kMaxSerialisedBytes` is 2,560).
- **Numbers** are written and read here, from integers in fixed decimals, so the watch needs no float `printf` or `strtod`. Values are rounded to the stored decimals as they enter the store (latitude and longitude 9, heading and R 4, altitude 2, quality 3) and read back with the same expression, so a round trip is bit-exact.
- **Damage.** A line is loaded only if it ends in a newline, its CRC-32 (of the line up to " crc=") matches, every required key (`n`, `lat`, `lon`, `hdg`, `r`, `cw`) is present and in range, and its number is not taken. A file cut off at any byte, garbled or empty gives an empty or partial store, never a crash. `load()` reports lines seen, tracks loaded, dropped lines by reason (cut off, CRC, bad field, duplicate), a missing header, and the version.
- **A later version** (header `v` above 1) loads every line with the version 1 keys, and the report says so (`newerVersion`). The service writes to a temporary file, moves the old one aside and renames the new one into its place (a rename does not overwrite on the watch), and does not save over a newer version's file.

### 7.2 Return visits: lining a stored track up with the day's GPS

The day's GPS can sit several metres from where it was when a track was stored, and drift tracking moves only 1 m a lap. A supplied model is therefore lined up first.

**The offset.** While lining up, each time the calibrator stores a point, a free 2D offset between GPS and the stored model is fitted to all the points so far, each against its own lane's line: Gauss-Newton on the lateral offsets, Huber weights at `alignHuberM` (5 m) times `hdopWeight()`, a weak prior that holds a direction no point has faced yet, and steps of at most 10 m. It waits for 6 points (about 18 m). Each solve starts both from the last answer and from no offset; the better fit wins, and where they fit within 25% of each other the smaller offset wins (where a straight meets a bend two offsets can fit about as well).

**What is used.** Each direction of the offset (the eigenvectors of the points' normal matrix) is used in full beyond `alignSigHi` (5.5) standard errors, not at all within `alignSigLo` (3.5), and in proportion between. A day's offset of several metres is used whole; a track stored in the right place is not moved by noise.

**When it ends.** Once the runner has gone `alignMinM` (100 m) along the track, the points face enough directions (the smaller eigenvalue of the sum of n nᵀ at least `alignCondFrac`, 0.2, of its trace: roughly the first bend), and no direction is undecided; in any case by `alignMaxM` (350 m), before lap 1 can end. Drift tracking then takes over, seeded with the offset. The off-track check waits while lining up.

**The start line** is the start point less the offset in use, and moves with it until lining up ends. Laps already counted never move: lap 1 needs a whole lap, and lining up ends first. The raw start point follows counting mode's until it is final, and freezes when the runner is under way (10 m along, or at once on a rolling start).

**Running direction.** A stored track carries the direction it was calibrated in. While lining up, and before lap 1 reaches `alignFlipMaxM` (100 m), the runner's own movement along the track (lightly smoothed positions against the stored direction) is summed; at `alignDirM` (30 m) against, the model is reversed and lap 1 starts again from the start line. Judging it from raw movement keeps it clear of the offset still being lined up. A 35 m run-up behind the line works.

**A wrong pick.** From `alignMinM` on, an offset in use beyond `alignMaxOffsetM` (35 m), or a robust scale beyond `alignMaxScaleM` (9 m), means the stored track is not this one. The scale limit widens with the root mean square HDOP of the points above 1.0 (`kAlignScaleRefHdop`); with no HDOP it stays at 9 m. The track is dropped: the mode goes back to Calibrating, counting mode (which has measured all along) shows the distance, and calibration carries on as on a new track.

**A turned neighbour.** A stored track 40 to 60 m away and turned 15 to 50 degrees can line up acceptably on offset and scale alone. Two checks catch it:

- **A turn test.** When lining up would end, the stored model is fitted again with a rotation about its centre as well as the offset (`solveTurn`: Gauss-Newton on the lateral offsets, Huber and HDOP weights, 8 steps of at most 5 degrees, a standard error from the normal equations and the robust scale). Turned 10 degrees or more and 4 standard errors (`kTurnLimitDeg`, `kTurnSigma`), on 200 m of points or more (`kTurnMinPathM`), by two tests in a row the same way round (`kTurnRuns`), it is another track and is dropped as a wrong pick. The test repeats every 10 points (`kTurnEvery`) until the track is confirmed. 10 degrees lies between a mis-stored heading (about 6 degrees; a right track's own locked fit is within 1.5) and a neighbour (15 or more).
- **The visit's first fit.** At the calibrator's first fit, the stored track lined up to all the points is compared with it: turned 10 degrees or more, it is dropped (`dropTurned`); otherwise it is confirmed. With no fit, two counting-mode laps confirm it (`kConfirmLaps`); a rejected fit confirms it too.

Until a stored track is confirmed, counting mode's laps are held (up to 4), the calibration keeps collecting even while the runner is judged off the track, and the Session's settle bound stays at Start. If the track is then dropped, laps already shown are re-timed to counting mode's time and length by `LapTimeRefined`, the total moves by the difference, and a lap counting mode has that the Tracker missed is credited late. A lap the wrong geometry counted that counting mode did not cannot be taken back. The model of a track not stored, visited earlier in the run, is the run's own fit, so it is confirmed from the start (`confirmOwnFit`).

**Figures** (`ReturnVisit.DailyOffsetsAndStartsRoundTheLap`: 160 runs, offsets of 0 to 20 m at 8 bearings, starts round the lap and mid-bend, noise 2 to 5 m with drift, 4 laps each):

| Offset | Lined up after | Offset error at the end | Start line moved | Part-lap 95th, lining up | Part-lap 95th, after | Lap 1 time, mean / 95th |
|---|---|---|---|---|---|---|
| 0 m | 125 m | 1.37 m | 0 m | 9.4 m | 5.8 m | 0.55 / 0.99 s |
| 5 m | 149 m | 1.32 m | 2.2 m | 8.8 m | 8.4 m | 0.51 / 1.45 s |
| 10 m | 128 m | 1.25 m | 6.1 m | 9.2 m | 7.3 m | 0.51 / 1.22 s |
| 20 m | 127 m | 1.62 m | 13.5 m | 11.7 m | 7.9 m | 0.56 / 1.34 s |

Every lap was counted and credited exactly. The start line moves by about the along-track part of the offset; lap 1's time is no worse than later laps', because it ends on the same refined line. The offset error is noise-limited (about 0.8 m at 2 m noise, 1.7 m at 5 m). Where part of an offset falls in the undecided band, drift tracking finishes it at 1 m a lap, which is why part-lap after lining up is 1.5 to 2.5 m worse with an offset.

**Neighbours** (`NeighbourSweep.TurnedNeighbourAt40And60m`, 300 runs a distance per seed set): a neighbour kept with wrong laps in 0% of runs on all four seed sets. A neighbour dropped after laps were shown (re-timed): 20 to 26 runs in 300 at 40 m, 3 to 6 at 60 m. With both tracks stored, the Session ended on the right track in 20 of 20 runs.

### 7.3 Checking a stored track on each visit

Calibration runs in the background on every run, a stored track included. On a stored track it changes nothing shown: it raises no calibration events.

**At the visit's lock** (lap 4) the visit's own fit V is compared with the stored track S after removing the day's offset (S is lined up with all of this run's points). Four figures: the heading and R differences, the centre difference, and how much worse S fits the visit's points than V does (the ratio of robust scales).

| | Heading | R | Centre | Scale ratio |
|---|---|---|---|---|
| Agrees (all of these): blend | ≤ 2 degrees | ≤ 1.0 m | ≤ 3 m | ≤ 1.25 |
| Clearly differs (any of these): replace | > 4 degrees | > 2.0 m | > 6 m | > 1.5 |

In between, the stored track stays and the visit counts as a use. When the visit clearly differs, the rest of the run also moves onto the visit's fit (`rebaseModel`, keeping the runner's place).

**The outcome** (`Tracker::storeOutcome()`, after `finish()`):

| Kind | When | What `apply()` does |
|---|---|---|
| New | No stored track and the run locked; a wrong pick that locked more than `storeReplaceNearM` (50 m) from the picked track; or a turned neighbour's own fit | `saveNew()` |
| Updated | The visit agreed | `blend()` |
| Replaced | The visit clearly differed; or a wrong pick that locked within 50 m of the picked track with a heading within 4 degrees of it | `update()`, keeping the number |
| Unchanged | A stored track was lined up and used, but the visit never locked or was in between | `touch()` |
| None | No lock, and no stored track used | nothing |

No save happens unless the visit's calibration locked. If the supplied track was forgotten meanwhile, `apply()` saves a locked fit as new. A wrong pick turned more than 4 degrees from the run's own fit never replaces the track it was picked as: that would overwrite a stored warm-up track with the main one.

**Figures.** Blending (10 sequences of 8 visits, each day with its own offset of about 3 m an axis): 70 of 70 later visits agreed; the stored centre's error against the true track fell from 3.90 m after the first visit (worst 6.74 m) to 1.30 m after the eighth (worst 2.28 m), and R from 0.33 to 0.09 m. A track stored at R 35 m and really 38 m differed by 2.4 to 2.7 m and was replaced every time; a first calibration 6 degrees out differed by 5.5 to 6.3 degrees and was replaced every time.

## 8. The lane: the runner's

The runner sets the lane. `Session::setLane(lane, tMs)` takes a lane (1 to 8) or `kNoLane` (0), from the start screen (as the lane given to `start()`), the pause menu or a hold of Lap on the run screen. The Session never changes the lane by itself, and nothing measures which lane the runner is in.

| Where | A lane | `kNoLane` |
|---|---|---|
| Start | On a track from Start (section 10.2) | On the road from Start |
| On a track | A lane change from here: the Tracker keeps the runner's station and measures on the new lane's line (section 6.3). The lap carries on. | Leaves the track here, or where the runner went off it if they have: the road from there (section 10.5) |
| On the road | Joins a track here (section 10.4) | Nothing changes |

- **The time.** `tMs` is the sensor clock at the press; a time before the latest reading is taken as the latest reading.
- **`ok`** is false for a lane not in the table. The events are those of the change: `SegmentChanged`, and a `TrackLap` the Tracker's `finish()` closes on leaving.
- **"No lane" on a track** keeps that track from being joined again by itself (section 10.3) until the runner has been away from every known track: no ring point within `storeNearM` (150 m) of a known track's centre (`mAwayFirst`), or sets a lane again. A warm-down or an infield session beside the track stays road. Once away, a join reaches back no further than that point.
- **The lane a join noticed takes** is the one last run on that track: the stored record's `lane`, or for a track visited earlier in the run the lane run there. A stored record with no lane takes the last lane run in this run (`mLastLane`, lane 1 before any). The GUI shows the track and lane for a few seconds, and the runner changes it with `setLane()` like any lane.
- **The lane the next run starts with** is the service's: the lane the runner set last, on the start screen, in the pause menu or by holding Lap ("No lane" included). A lane a join noticed takes is not the runner's, and is not kept.

## 9. HDOP as the precision figure

`getPrecision()` is HDOP, the horizontal dilution of precision: a unitless measure of the satellite geometry, about 1 when good and 2 or more when poor. Track Run reads it as HDOP, not as an accuracy in metres: the track logic treats it as HDOP throughout.

**The helpers** (in `Config.hpp`):

| Helper | Value | Used by |
|---|---|---|
| `hdopErrorM()` | HDOP x `uereM` (4 m); no HDOP: `unknownHdop` (1.5) x 4 = 6 m | Start point weights, 1 / error squared |
| `hdopRelError()` | The error over that of a reading with no HDOP: HDOP / 1.5, HDOP taken as at least 0.5; exactly 1 with no HDOP | `hdopWeight()`, `hdopWiden()` |
| `hdopWeight()` | 1 / `hdopRelError()` squared: 2.25 at HDOP 1, 1 at 1.5, 0.25 at 3, 0.14 at 4, at most 9 | Every weighted fit: calibration, lining up, the turn test, drift tracking |
| `hdopWiden()` | `hdopRelError()`, never below 1: 2 at HDOP 3, 2.67 at HDOP 4 | The off-track distance, "close to the line", the outlier gate, the exit distance |

- **`uereM`** is the app's assumed error per unit of HDOP. Every use in the track logic compares readings with each other, or with a reading with no HDOP, so it cancels out; it matters for converting version 1 traces and for anything that shows the error in metres.
- **A reading with no HDOP** has relative error and weight exactly 1, so a run with no HDOP is weighted evenly throughout. The golden file (section 13.5) pins this.
- **Distances only widen**, never tighten: they were set with drift and runners' lines in mind, not just noise.
- **Left unweighted on purpose:** the calibrator's robust scale, shape and coverage checks (so their limits keep their meaning), the refined lap-time fit (a reading over HDOP 4 never enters it), the counting-mode distances, the visit check's limits and `TrackStore`.

**Poor-HDOP spells** (`Hdop.PoorSpellsOnAStraightABendAndAcrossTheGate`): spells of HDOP 3 to 5 for 20, 40 and 60 s, on the back straight, the second bend and the gate, in lap 2 (calibrating) and lap 5 (calibrated), 108 traces each through the Tracker from scratch, on a stored track and through counting mode alone. No false or missed laps in any run, and every lap credited its lane's length exactly. About 40% of the spell readings were over HDOP 4 and dropped. Lap ends against the true pass: on real crossings mean 0.48 s, 95th percentile 1.14 s, worst 1.75 s; across a gap mean 0.90 s, 95th 2.13 s, worst 6.39 s.

## 10. Road and track in one run (`Session`)

Many runners run to the track as a warm-up, do their session, and run home, all as one activity. A `Session` treats a run as a sequence of segments, each Road or Track. Road distance is the kernel's GPS distance; track distance is what the Tracker measures.

### 10.1 Design: a Session that owns a Tracker, and joins by replay

A Tracker's local frame is centred on its start point, and the calibrator stores points as `int16` decimetres (plus or minus 3.2 km). A track 4 km from home cannot share a frame with home. So every track visit is a fresh `Tracker::start()` at the join point. A lane the runner sets starts it at the latest good reading. A known track the Session notices (section 10.3) is joined some seconds after the runner came onto it, so it is started back there:

- The Session keeps a **ring of recent points**: one every `ringSpacingM` (3 m) of GPS distance, 376 points (about 1.13 km). Each is 12 bytes: decimetres in the Session's own frame (re-centred on the runner when more than 2.5 km from its origin), the time in whole milliseconds (two neighbours can be hours apart, across a pause), HDOP in twentieths, and the GPS distance since the previous point in decimetres, up to 6.5 km (rounded with the error carried on, so the sum never drifts).
- With the ring it keeps the runner's last 32 pauses, resumes, Lap presses and lane changes on a track, so a replay gives its new Tracker each at its own time: paused time stays out of the replayed laps, a lap restarted stays restarted, and a visit restarted on another stored track measures each stretch on its own lane. When it is full, the oldest Lap press gives way first, so the pauses and lanes stay.
- To join a track at a ring point, the Session primes a fresh Tracker with that point, starts it there, and **replays** the points since, rebuilt as readings. The Tracker sees a run that started at the join point.
- So laps count from the join point, because that is the start line; a stored track is lined up with the day's GPS exactly as at Start; and a track not stored is calibrated exactly as from Start. No Tracker rule is duplicated.

**Totals.** Road distance is the Session's own odometer (as counting mode reads the kernel's distance: deltas cut to 12 m/s, the step between positions when there is none, nothing while paused) over the Road segments. Track distance is the sum of closed Track segments plus the current Tracker's total. The shown total is road plus track and never decreases; where a correction would lower it, it holds.

### 10.2 Starting a run

- **No lane (`kNoLane`):** a Road segment from Start, with no Tracker. A known track met on the way is joined by itself (section 10.3); any other track when the runner sets a lane (section 10.4).
- **A lane, near a stored track** (`findNear` on the primed position): a Track segment from Start, the stored model supplied. If it is dropped as a wrong pick before any lap and another stored track lies within `storeNearM` of where the visit began, the Session starts again on that one (`restartOther`, section 10.4). Otherwise the visit carries on as a track that is not stored, counting mode having measured from Start.
- **A lane, no stored track nearby:** a Track segment from Start, on a track that is not stored. Counting mode measures from Start and calibration learns the track as on any new track.

A run that starts on a track raises no `SegmentChanged` at Start; `segment()` says Track from the first call.

### 10.3 Arriving at a known track

A known track is a stored one, or one visited earlier in this run (kept with its model lined up with today's GPS, for a rejoin). On the road the Session watches for one by itself, on every new ring point while not paused.

- **Near it:** on each new ring point, the nearest known centre within `storeNearM` (150 m).
- **On it:** the model is placed in the Session's frame and the newest points are projected onto its kerb line. Over the newest `joinConfirmM` (40 m) of path, the median offset from the kerb must be within the band, at least three quarters of the points in it, the net movement along the track at least `joinAlongFrac` (0.7) of the path, and the runner moving. The band runs from 10 m inside the kerb (`joinMarginM` plus 4 m) to `joinMarginM` (6 m) beyond lane 8's outer edge, 15.8 m from the kerb. A path 15 m outside lane 8 is 24.8 m out; a day's GPS offset inward should not hold up a runner in lane 1 or 2.
- **The join point** is where the runner came onto their own line: back from the newest point while within the stretch's median offset, plus or minus 2.5 times the stretch's robust noise (at least 6 m); three stray points in a row, or ring points more than 20 s apart (`kJoinGapMs`: a pause, a GPS outage), end it. The band decides only whether the runner is on the track, not where they came on: a runner's line near its edge (lane 8 with a day's offset outward) still reaches back to the entry. Too wide and a slanting approach across the lanes is taken as track (its GPS distance would be lost); too narrow and noise ends it early.
- **The entry.** The readings either side of the join point (up to 6 ring points before it, those within 30 m of the runner's line, and 10 after) are fitted with a hinge (`entryAt`): the lateral offset closes on the runner's line at a steady rate until time te, then stays on it; the station runs along two straight legs meeting at te. Only the approach's last 4 s are used (`kEntryLegS`), since a road can turn just before the track. te is searched from 3 s before the join point to 6 s after it (`kEntryLateS`), every 0.5 s and then every 0.05 s. It is used only for a clear slant: crossing at 0.5 m/s or more (`kEntrySlopeMps`) and fitting 16 times the noise better (`kEntrySigma2`) than "on one line at one speed". Otherwise the join point's own time stands. A single reading's station carries its noise along the track; the fit averages it out.
- **The join:** road distance is closed at the entry; a Tracker is started there with the known model (reversed if the runner is going the other way) on the lane last run there (section 8), and the ring is replayed from the first point after it; `SegmentChanged` (timed at the entry) and `TrackJoined` are raised, and `autoJoins()` counts it. The display holds rather than drops.
- **If lining up rejects the model** during the replay, the join is cancelled before anything is shown, and no join is tried for `joinRetryM` (200 m). If it rejects it later, in the first lap, the join is undone: `SegmentChanged` back to Road with the same `tMs` as the join, and the road carries on as if it had not happened. A stored track turned from the one picked stands as a track that is not stored.

### 10.4 A lane set on the road

The Session never takes a loop that is not a known track for a track by itself. A track that is not stored is joined only by the runner setting a lane (`joinHere()`):

- **Where:** at the latest good reading, at `tMs` (while paused, at the pause: nothing was run since). Road distance is closed there, and `SegmentChanged` from Road to Track is raised at `tMs`. Laps count from there. With no good reading in the last `kGoodFreshMs` (3 s), the lane waits and the join is made at the next good reading; "No lane" before then cancels it.
- **Which track:** the known track whose centre is nearest the latest good reading, within `storeNearM` (150 m), with its model lined up with today's GPS as at Start (a track visited earlier in the run is its own fit, confirmed at once). With none, a track that is not stored: counting mode from there, and calibration learns it as on a new track (locked at lap 4, stored as new at the end).
- **The lane** is the one set.
- **A wrong pick.** A known track dropped as a wrong pick before any lap is replaced by another stored track within `storeNearM` of where the visit began, if there is one: a fresh Tracker, primed with counting mode's start point and started at the visit's start, then the ring's readings and the runner's pauses, resumes and Lap presses since replayed (`restartOther`, once a visit; the same at Start). Otherwise the visit carries on as a track that is not stored. A pick the runner is off while it is still being lined up, without having been on it since the lane was set (on another track beside it), is dropped as a wrong pick at once, by the exit rules below: the visit carries on in the same way. A join the runner made is not undone, except by the runner: "No lane" before the next reading leaves no visit at all.
- **While paused** the Tracker starts paused at `tMs`, and runs from the resume.

### 10.5 Leaving a track

- **When.** The Tracker's off-track check decides (12 m beyond the lanes, 3 readings, widened by HDOP): the exit is confirmed once the Tracker has been off for `offTrackMs` (30 s), or sooner once 3 readings in a row are beyond `exitFarM` (50 m, also widened; no infield cut goes that far). While a stored track is still being lined up, when the Tracker's own check waits, the `exitFarM` rule alone applies. With no fitted track (counting mode), the runner has left once beyond `exitCountingM` (250 m) from where the visit began.
- **Where.** The track segment ends where the runner was last on the track: the Tracker's last reading close to the line that still moved along it, if within 20 s of the runner being confirmed off, else the first reading off (section 6.3) (while lining up, the last reading within `offLanesM`, 12 m times `hdopWiden()`, beyond the lanes; in counting mode, the last reading within `exitAwayM`, 190 m, of where the visit began). The part-lap keeps its track distance up to there. The road segment starts there, so the GPS distance over the confirmation window is road distance.
- **A short exit** (under the window) is left to the Tracker: the way along the track is credited at the earlier speed (section 6.3).
- **While paused** nothing leaves the track: a runner who walks off during a pause (to a tap, a bag) is judged afresh from the resume, when the rules above start counting again. Back on the track, the visit and the lap in progress carry on.
- **"No lane"** from the runner leaves the track at `tMs`, at the odometer then, or where the runner went off the track if they have (section 8). The part-lap keeps what was measured.
- **Rejoining the same track** after a confirmed exit is a new join, with the model as it was lined up today: laps restart from the rejoin point, the run's lap count carries on, and it is lined up again.
- **The lane** after leaving is `kNoLane`.

### 10.6 `lapPress()` and the events as splits

Track Run does not call `lapPress()`, and takes no split from these events: its laps are Run's (the Lap button and auto-lap, by distance or time), and it takes only the total from the Session. They are there for a caller that wants track laps and road kilometres as splits.

- **On a track** `lapPress()` restarts the lap from where the runner is: the next lap ends a full lap from there. What was run of the lap is credited as distance, with no lap. In calibrated mode the start line moves to the furthest point reached; in counting mode the gate re-forms round the latest position as at Start. A lap time waiting to be refined is settled first.
- **On the road** the next road split is a whole `roadSplitM` from the press.
- **Splits.** `RoadKilometre` each whole split of a road segment; `TrackLap` each lap; `SegmentChanged` at each change, so warm-up, session and cool-down can be separate splits.

### 10.7 Late events and the settle bound

- **Late segment changes.** `SegmentChanged` is timed where the runner came onto the track (some seconds back for a known track the Session notices; at the press for a lane the runner sets) or left it (up to 30 s back). A `RoadKilometre` timed after the `tMs` of a later `SegmentChanged` is void, and the caller drops it.
- **Undone joins.** A `SegmentChanged` from Track back to Road with the same `tMs` as the join before it means the join did not stand.
- **`segmentStartTotalM()`** is the run's total at the latest segment change. A `SegmentChanged` event's own `totalM` is taken when the event is raised, after any replay that credited laps from the join, so it is not that figure.
- **`earliestRetimeTMs()`**: no event raised from now on is timed before it. On a track: the start of the visit while a supplied model is still being lined up or confirmed; the last reading on the track; the time the runner went off; in counting mode the last reading near where the visit began; and the latest lap end, Lap press or join less 15 s (a lap refined, or credited after a gap or a pending pass, ends after the lap before it). On the road: the oldest point of the ring, since a join the Session notices, and the laps credited with it, start at a ring point. A caller that takes the events as splits settles them against it.

### 10.8 Calibration, storing and several tracks

- Calibration, lining up and the visit check run only inside Track segments, since they are the Tracker's.
- Each track visited is remembered (up to `kMaxVisited`, 2): its store number (0 if not stored), the lane last run there, its laps, and its model lined up with today's GPS. At each exit and at `finish()`, the visit's `storeOutcome()` is merged: an outcome with a model (New, Updated, Replaced) beats Unchanged, which beats None, and the later visit wins a tie. For a track not in the store, a locked fit is New, whatever model the visit started with.
- **End-of-run match.** A run that dropped its stored track and locked its own fit is matched to a stored track within 50 m, 4 degrees and 2 m of R (Updated for that number); otherwise it is New.
- `storeOutcome(number)` gives the track with the most laps; `storeOutcome(i, number)` each of `visitedCount()`. A third track in one run replaces the one with fewer laps.

### 10.9 Figures

**Scenario tests** (`Session_test.cpp`, 2 to 6 seeds each; each starts with no lane unless it says otherwise):

| Test | Result |
|---|---|
| `RoadTrackRoadOnAStoredTrack`: 4 km road, 6 laps, 4 x 400 m with a Lap press each rep and 60 s recoveries, 4 km home, noise 3 to 5 m, no input | Joined and left by itself; all 10 laps exact; road splits right; elevation held on the track only |
| `RoadTrackRoadOnATrackNotStored`: the same on a track never stored, the lane set on arriving | All 10 laps exact from the lane set; locked and stored as new |
| `ALaneSetOnTheRoadJoinsTheStoredTrackThere`: lane 3 set on arriving at a track stored with lane 1 | That stored track, on lane 3, from there; every lap exact |
| `NoLaneOnATrackIsTheRoadUntilAway`: 3 laps, "No lane", 2 more laps on the track, 400 m away and back | The road from the "No lane"; not joined again on the track; joined again on coming back: 5 laps |
| `NoLaneAtStartIsTheRoad`: no lane, 4 laps of a track not stored | No laps; the total is the GPS distance to 0.05 m |
| `StartWithALaneOnATrackNotStoredMatchesTracker`: a lane at Start, no store, a pause and a lane change | Laps, lap times and total equal a Tracker's on its own, reading for reading |
| `RoadOnlyNeverFindsATrack`: a park loop 4 times, roundabout laps, a figure of eight, an out-and-back, a car park | No laps, no join; the total is the GPS distance to 0.05 m |
| `PathAlongsideTheTrackIsNotJoined`: 15, 20 and 25 m outside lane 8 of a stored track | Never joined |
| `ToiletDetourIsRoadAndLapsRestart`: 3 laps, a 3-minute 300 m walk off and back to another point, 3 laps | The detour is road; laps restart from the rejoin; the run's laps carry on |
| `APauseJustAfterArrivingStaysOutOfTheLaps`: a 5-minute pause 20 m onto a stored track | No lap holds the pause |
| `LaneOnArrival`: a stored track last run in lane 3 and in lane 1 | Joined on the lane stored; lane 5 set after the notice stands from there |
| `LapPressRestartsTheLap`: a press at 1.3 laps | The next lap ends within 2.5 s of the true pass of the press point, exactly 400 m |
| `TrackNotStoredInLane8WithGpsDistanceOver`: lane 8, GPS distance 13% over, 2.6 m/s, the lane set on arriving | Every lap exact |
| `NoLaneFromACarParkThenALaneOnTheTrack`: Start 100 m from a track, a jog onto it, the lane set, 6 laps | Joined within 1.5 s of the true entry; all 6 laps exact |

**The mixed sweep** (`SessionSweep.MixedRuns`, `TRACKRUN_SESSION_RUNS`, `TRACKRUN_SESSION_SEED`): 35% road to a stored track and back (joined and left by itself, stored with the runner's lane), 25% road to a track not stored and back (the lane set on arriving), 20% track only (a lane at Start, stored or not), 20% road only with no lane (with a park loop, roundabout, car park or figure of eight in half). Lanes 1 to 8, noise 2 to 5 m, a day's offset up to 5 m, road 600 to 2,500 m each way at 2.8 to 4 m/s, 3 to 7 laps at 3.5 to 6 m/s, leaving 120 to 280 m into a lap. `LargeDailyOffsets` is the same with offsets up to 20 m.

| 200 runs, seed set 0 | Offsets to 5 m | Offsets to 20 m |
|---|---|---|
| Runs failed (false entries or exits, false or inexact laps) | 0 | 0 |
| Total against the truth, mean | -2.0 m | +2.3 m |
| Road against the GPS distance on the road, median | 4.1 m | 4.4 m |
| Join placed from the true entry, median | 0.0 s | 0.0 s |
| Exit placed from the true exit, median, 95th percentile | 0.7, 2.5 s | 0.4, 2.3 s |
| Missed laps, decreases of the total | 0 | 0 |

A stored track is joined (`TrackJoined`) 7.0 s after the true entry at the median and 33 s at the 95th percentile, with offsets up to 5 m; with offsets of 15 to 20 m, 23 s at the median. Road distance against the route's true length carries the synthetic GPS distance's own over-read, about 20 to 25% at 3 m/s.

**Lap 1 at a join** (`StartSweep.JoinLap1AtEveryApproach`: a stored track the Session notices, and a track not stored whose lane the runner sets on arriving; noise 2 to 4 m, approaches from 12 m back and 22 m out, steep, shallow and along the lane, and a start with no lane in a car park 80 to 120 m from a track not stored; 30 runs a case), lap 1's refined credit less the metres run, seed set 0:

| | Mean | 95th percentile |
|---|---|---|
| The runner's lane (slanting approaches and the car park start) | +1.8 m | 10.2 m |
| Stored track (slanting approaches) | -0.9 m | 8.0 m |

A lane the runner sets is placed at the press, so its join is the true entry to the reading; lap 1 starts at the latest reading, whose noise it carries. A stored track's join is placed -0.0 to +0.2 s from the true entry at the median on the slanting approaches. Laps 2 to 4 are within 0.7 m on average.

## 11. Starts

The start-point rules are in section 5.1; this section collects how each kind of start comes out.

| Start | What happens |
|---|---|
| Standing, primed | Start point is the HDOP-weighted mean of the last 5 s of fixes |
| Standing, no primed fixes | First good fix after Start |
| A stale fix at Start (jump of 20 to 50 m) | The first readings move the start point; judged from the press estimate if the runner sets off at once |
| Rolling start (moving at 1.5 m/s or more, clearly) | Start point is where the runner was at the press; on a stored track lap 1 is armed at once |
| Walking start (1.0 to 1.5 m/s) | Treated as standing in almost every run: lap 1 starts at the primed mean, a few metres behind the press |
| A lane at Start on a track not stored | A track from Start, learnt as on any new track (section 10.2) |
| No lane at Start | The road; a known track met later is joined by itself, any other when the runner sets a lane |
| Lap 1 at a join | Credited from the entry, or from the press for a lane the runner sets (sections 10.3 and 10.4) |

**Rolling starts** (`StartSweep.RollingStartCreditsLap1FromThePress`: 2.5 to 8 m/s, lanes 1 and 8, 3 m noise, 40 runs a route and seed set):

| Seed sets 0 / 1 / 2 / 3 | |
|---|---|
| Stored track: lap 1 over-credit, mean | +0.1 / +1.8 / +0.2 / -0.4 m |
| Stored track: lap 1, 95th percentile of the error | 4.5 / 8.9 / 6.8 / 6.3 m |
| Stored track: the total when first shown, ahead of the metres run (95th) | 1.1 / 4.0 / 0.6 / 0.2 m |
| Track not stored: lap 1 over-credit, mean | +2.3 / +3.0 / +1.4 / -0.8 m |
| Track not stored: 95th percentile | 8.3 / 11.7 / 8.4 / 8.0 m |
| Wrong laps | 0 |

The rolling-start rule keeps the 2.5 to 3 s of running between the primed fixes and the press out of lap 1 on a stored track.

**Jumps at Start** (`StartSweep.JumpsAtStartStandingAndRolling`: jumps of 10 to 50 m in eight directions, 2 to 3 m noise, standing 3 s, setting off at once, or rolling in; counting mode and the Tracker from scratch, 288 runs a seed set): no run with a lost or false lap on any set, and every start point over 20 m out corrected to within 10 m. Corrected start points are within 2.8 to 7.7 m of the press at the 95th percentile. Jumps up to 15 m are kept by design (they are within the noise allowance); the worst lap end there is 2.0 to 3.9 s out.

**Walkers.** At 1.0 m/s a walker is never judged rolling, at 1.2 m/s in 0 or 1 of 8 runs, at 1.5 m/s in 0 to 3 of 8: the primed fixes do not move clearly enough over 2.5 s against 3 m noise. Lap 1 then credits 3 to 6 m more than was walked on a stored track (mean over the seed sets; worst 11 m), and 4 to 8 m in counting mode (worst 13.5 m).

## 12. Configuration

Every threshold the service can tune is in `Oval::Config`, so replayed traces can tune them without code changes. A few finer constants live in the `.cpp` files (`k...`, named where they are described above).

| Field | Default | Meaning |
|---|---|---|
| `maxHdop` | 4.0 | Readings with a higher HDOP make no position decision. About 40% of the readings in a spell of HDOP 3 to 5 exceed it. |
| `unknownHdop` | 1.5 | HDOP taken for a reading with none; the reference weights are relative to |
| `uereM` | 4 m | Metres of error per unit of HDOP |
| `primeWindowMs` | 5 s | Primed fixes this recent are averaged into the start point |
| `staleStartM` | 15 m | The first readings this far from the primed point mean the primed fixes were stale: above 5 m noise once averaged, below the 20 m and larger jumps the start tests use |
| `gateDirDistM` | 20 m | The gate direction is taken from the runner at this distance |
| `gateDirConfirm` | 2 readings | At 5 m noise, a standing runner's reading lands 20 m out about once in 3,000 readings; two in a row about once in ten million |
| `gateResetM` | 10 m | Back this close to the start point, the move away was not the start of a lap |
| `gateRadiusM` | 25 m | A crossing this close to the start point counts at once |
| `maxCrossGapMs` | 10 s | A crossing over a longer gap becomes a pending pass; also the gap that allows distance-based credits |
| `armFarM` | 40 m | A gate-line crossing this far out is on the far side: it makes the gate final, and arms it |
| `passRadiusM` | 35 m | A pass this far out is still the runner going by the gate; kept below `armFarM` |
| `minLapFrac` | 0.70 | A lap needs this fraction of a lap of scaled GPS distance |
| `minLapMs` | 40 s | A lap needs this much active time. The 400 m world record is 43 s. |
| `missedLapFrac` | 1.5 | A pending pass is credited at this fraction of a lap; a longer lap after a gap first credits the hidden lap |
| `missedLapNoPassFrac` | 2.5 | Distance alone, after a gap: late enough not to fire on detours or GPS distance that reads long |
| `ratioMin`, `ratioMax` | 0.80, 1.25 | Bounds on true metres per GPS metre |
| `partLapCap` | 0.98 | The part-lap estimate never exceeds this fraction of the lap |
| `maxSpeedMps` | 12 m/s | GPS distance faster than this is cut |
| `calSpacingM` | 3 m | Calibration point spacing; doubles when the buffer is thinned |
| `calProvisionalLaps`, `calLockLaps`, `calMaxLaps`, `calRejectLaps` | 1, 4, 6, 3 | Laps of coverage for the provisional fit, for locking, the most before a fit counts as disagreeing, and from which a failed check rejects |
| `lockCentreM`, `lockHeadingDeg` | 1.5 m, 1.5 degrees | Successive fits agree when this close. Observed moves at lap 4: at most 1.22 m and 1.10 degrees. |
| `fitRadiusMinM`, `fitRadiusMaxM` | 32.5, 39.75 m | Search range for R; a fit on its edge is rejected |
| `maxShapeM` | 2.0 m | Noise-corrected shape mismatch limit; standard tracks at most 0.78 m from lap 3 |
| `maxScaleM` | 9 m | Robust residual scale limit; standard tracks at most 5.4 m |
| `minBinPoints` | 2 | Points needed in each twelfth of the fitted track |
| `abAlpha`, `abBeta` | 0.5, 0.12 | Alpha-beta filter on the along-line position |
| `outlierM` | 20 m | Outlier gate, widened by HDOP, plus 6 m/s times the gap |
| `stillMps` | 0.5 m/s | Below this smoothed speed, progress waits |
| `offLanesM`, `offTrackM`, `offTrackReadings`, `offTrackMs`, `rejoinReadings` | 12 m, 15 m, 3, 30 s, 5 | Off the track (beyond the lanes) and back (within three quarters of `offLanesM`); where the runner left (within a third of `offTrackM` of their line). Three readings in a row: at 4 to 5 m noise with drift, none of 10,000 generated runs raised a false alarm. |
| `switchFromFrac`, `switchToFrac` | 0.25, 0.75 | The mid-run switch waits for this part of the lap |
| `driftPerLapM` | 1 m | Drift tracking moves at most this a lap. 3 m a lap changed the part-lap figures by under 0.15 m. |
| `refineBeforeMs`, `refineAfterMs` | 5 s, 3 s | The refined lap time's window |
| `storeNearM` | 150 m | A stored track is recognised this close |
| `storeReplaceNearM` | 50 m | A wrong pick's locked fit this close replaces the picked track; further, it is new |
| `storeBlendMaxWeight` | 9 | A visit always counts at least 10% |
| `alignMinM`, `alignMaxM` | 100 m, 350 m | Lining up takes roughly the first bend, and always ends before lap 1 can |
| `alignCondFrac` | 0.2 | The points face enough directions: about half a bend |
| `alignHuberM` | 5 m | Down-weights readings far from the line while the offset settles |
| `alignMaxOffsetM`, `alignMaxScaleM` | 35 m, 9 m | Beyond these the stored track is a wrong pick; the scale limit widens with HDOP |
| `alignDirM`, `alignFlipMaxM` | 30 m, 100 m | Running the stored track the other way round. A run-up or jogging back to the line rarely goes 30 m back. |
| `alignSigLo`, `alignSigHi` | 3.5, 5.5 standard errors | How much of each direction of the offset is used |
| `visitAgreeHeadingDeg`, `visitAgreeRM`, `visitAgreeCentreM`, `visitAgreeScaleRatio` | 2 degrees, 1.0 m, 3 m, 1.25 | The visit agrees with the stored track: blend |
| `visitReplaceHeadingDeg`, `visitReplaceRM`, `visitReplaceCentreM`, `visitReplaceScaleRatio` | 4 degrees, 2.0 m, 6 m, 1.5 | The visit clearly differs: replace. Changed tracks in the tests measured at least 2.4 m of R or 5.5 degrees. |
| `joinMarginM` | 6 m | The join band beyond lane 8's outer edge (and 4 m more inside the kerb) |
| `joinConfirmM` | 40 m | Path along a known track to join it |
| `joinAlongFrac` | 0.7 | Moving along the track, not across it |
| `joinRetryM` | 200 m | No join after a cancelled or undone one for this much path |
| `exitFarM` | 50 m | Leaving sooner than 30 s when clearly away. The infield's middle is at most about 38 m inside the kerb. |
| `exitAwayM` | 190 m | Counting mode only: the track lies within this of where the visit began |
| `exitCountingM` | 250 m | Counting mode only: this far from where the visit began has left the track. A 400 m track's far end is at most about 180 m from any point on it. |
| `ringSpacingM` | 3 m | Ring point spacing |
| `roadSplitM` | 1,000 m | Road split length (`setRoadSplitM` sets a mile on an imperial watch) |

## 13. Tests

The host tests are in `Tests/Host/`; how to build and run them is in [README.md](../README.md). The default run (`trackrun-host-tests`) leaves out the `Targets` suite.

### 13.1 The synthetic generator (`TraceGen`)

`TraceGen` moves a runner along the measurement line of a lane on a standard track, or along any closed path for the non-standard shapes. You choose the track's centre, orientation and bend radius, the lane, the start point, the speed (steady or varying, slower on bends) and the clock start (which can wrap during the run). The runner can also stand, jog back and forth, take a detour, change lane (declared or not), set the lane with no move, pause, drop out, leave the track and rejoin further on, start rolling (`beginRolling`), and run road routes (`beginAt`, `road`, `joinTrack`, `leaveToRoad`, `lapPress`). Road-like shapes: a roundabout, a figure of eight, a car park perimeter, an out-and-back and a park loop.

- **Positions** carry white noise per axis plus a random-walk drift, and an optional steady drift ramp. They are rounded to `float`, as the sensor gives them.
- **HDOP.** By default log HDOP wanders as a first-order autoregressive process (standard deviation 0.2, time constant 2 minutes) round 0.96, clamped to 0.7 to 1.5 and given to 0.1; its root mean square is 1.0. A reading's white noise is the noise figure times its HDOP, so each test's overall noise band is kept. Spells of HDOP 2 to 5 for 20 to 60 s can be random (`hdopSpellsPerHour`, off by default) or scripted (`poorHdop`). Bad readings (`badFixProb`) report HDOP 6 to 10. Other models: no HDOP at all, and a fixed figure for tests whose thresholds were set without HDOP.
- **GPS distance** sums steps between the generator's own noisy positions, with their own jitter and a scale factor, so over-read can be set apart from position noise. Summing the 3 to 5 m white noise of the positions would over-read by 40 to 100%.
- **Truth.** When the runner really passed the start point, what each lap should credit, track distance as forward progress along the line actually run, each lap's physical length, and on Session routes, per reading, track or road, the true road distance, the GPS distance on the road and the station.

`Replay` runs a trace through a `LapCounter`, `Tracker` or `Session`, with the trace's commands (pause, resume, the lane set, Lap presses; `TraceGen::Generator::setLane()` sets a lane, or "No lane", with no move) and a hook after each reading, and checks after every reading that the total never decreased. `TraceCsv` reads and writes the trace format (docs/GPS_TRACE_FORMAT.md), so a recorded trace replays the same way.

### 13.2 Portable random numbers and maths

The tests give the same traces on every compiler and maths library:

- **`PortableRandom.hpp`.** The standard distributions are left to each library by the C++ standard, and MSVC and GCC give different uniforms and normals from the same `std::mt19937`. The portable versions take uniforms from 53 raw bits, integers by a 64-bit multiply-shift and normals by Marsaglia's polar method, with the same call shape as the standard classes. Each draw is its own statement, since the order of several draws in one argument list is the compiler's.
- **`PortableMath.hpp`.** `ln`, `exp`, `sin`, `cos`, `atan2` and `hypot` built only on `+ - * /`, `sqrt` and exact operations (Cody-Waite reduction with fdlibm's split constants, then short series). The generator and the sweeps' own geometry use them, and TraceGen has its own copy of the local-frame projection with the portable cosine.
- **Build flags.** `-ffp-contract=off` for GCC and Clang (a fused multiply-add rounds differently), and a static runtime for MinGW.
- `PortableRandom.PinnedValues`, `PortableMath.PinnedValues` and `PortableRandom.PinnedTrace` (a hash of a ten-lap trace with every kind of noise) pin the exact bits. `PinnedTrace` passes with MSVC (Debug, Release), MinGW GCC, GCC and Clang on glibc x86-64, and GCC on glibc AArch64.

The track logic's own `float` maths (`atan2f`, `cosf` and the like) still differs in the last digit between C libraries, and the watch's library differs again. That is why the golden file compares within tolerances (section 13.5).

### 13.3 Seed sets

`TRACKRUN_SEED_SET=1`, `2` or `3` moves every randomised test onto another set of draws; 0, the default, is the suite's own. Some single-scenario tests have limits set on one draw, so they pass on set 0, where they are asserted, and a few fail on other sets: `LapCounter.NoiseOf5mWithDrift`, `LapCounter.FinishCreditsAPassStillPending` (a lap end 5.1 s out against 4 s), `Tracker.PartLapAccuracyAcrossNoise`, `Tracker.LeavingAndRejoiningDoesNotCreditTheDetour`, `Tracker.ShortDetourIsFrozenAndNotCredited`, `Tracker.RefinedLapTimesBeatProvisionalOnes` and `Tracker.GrowingDriftAfterLockIsTracked` (statistics just over their limits) and `Calibrator.AccuracyAcrossNoiseHeadingsAndRadii` (one run's centre 1.93 m from the GPS position against 1.5 m).

### 13.4 The sweeps

| Sweep | Size (default; at scale) | What it randomises |
|---|---|---|
| `Sweep` (counting mode) | 200; `TRACKRUN_SWEEP_RUNS=5000` | Lane 1 to 8, 3 to 7 m/s, noise 2 to 5 m with drift, GPS distance over-read -3% to +26%, orientation, location, start point, clock start, 8 to 12 laps |
| `TrackerSweep` | 200; `TRACKRUN_SWEEP_RUNS=5000` | The same, half from scratch and half with a stored model and a random start; also `StoredTracksWithDailyOffsets`, all on a stored track with a day's offset up to 20 m |
| `SessionSweep` | 200; `TRACKRUN_SESSION_RUNS=5000` | Section 10.9 |
| `NeighbourSweep` | 300 a distance; `TRACKRUN_NEIGHBOUR_RUNS` | Section 7.2 |
| `StartSweep` | fixed grids; `TRACKRUN_JOIN_RUNS` | Sections 10.9 and 11 |

**Results at 5,000 runs**, seed sets 0 / 1 / 2 / 3:

| | |
|---|---|
| Counting mode: runs failed | 0 / 1 / 0 / 0 |
| Counting mode: lap time error, 95th percentile | 1.94 / 1.91 / 1.90 / 1.95 s |
| Tracker: runs failed | 0 / 1 / 1 / 0 |
| Tracker: standard tracks rejected | 1 / 0 / 0 / 1 |
| Tracker: part-lap 95th percentile | 6.54 / 6.49 / 6.42 / 6.48 m |
| Stored tracks with offsets to 20 m: runs failed; right tracks dropped as wrong picks | 1, 1 / 1, 0 / 1, 0 / 0, 0 |
| Stored tracks: part-lap 95th percentile | 7.56 / 7.55 / 7.52 / 7.52 m |
| Mixed runs: section 10.9 | |

No sweep has a missed lap. In the 200-run default sweeps every run is clean on all four seed sets: no failed runs, no wrong picks, no drops, no false entries or exits, no false or missed laps. In the 200-run counting sweep every lap is counted (1,971 laps) and the total at the final crossing is exact to 0.01 m; its lap time error is 0.70 s mean, 1.96 s at the 95th percentile, 6.22 s worst.

### 13.5 The Targets suite and the golden file

**Targets** are goals the track logic does not meet yet, with their thresholds kept. They run and print their figures, outside the default run: `TRACKRUN_TARGETS=1 ctest --test-dir Tests/Host/build -C Release -L targets -V`, or `trackrun-host-tests --gtest_filter=Targets.*`.

| Target | Where it stands |
|---|---|
| `CalibrationRadiusWithin0_5mAtLock` (and `WithoutDrift`) | 95th percentile 0.76 m with drift (37 of 200 runs over 0.5 m), 0.71 m without (25 of 200) |
| `CalibrationHeadingWithin1DegreeAtLock` (and `WithoutDrift`) | 1.09 degrees with drift; 0.86 without, reached |
| `PartLapFromScratchWithin5mAt3mNoise` | 95th percentile 5.86 m |
| `PartLapStoredModelWithin5mAt3mNoise` | 5.14 m |
| `JoinLap1Within5mAt95th` | 8.0 m on a stored track, 10.2 m for the runner's lane (seed set 0) |

**The golden file.** `Hdop.NoHdopAtAllMatchesGolden` runs two scenarios with no HDOP (a run from scratch with a pause and a lane change; a stored track with a day's offset and a move of four lanes the runner sets) and compares every event, plus the total and mode at every third reading, with `Tests/Host/data/hdop_unknown_golden.txt` (814 lines). Indices, event types, lap numbers and modes must match exactly, metres within 1 mm and times within 1 ms: the C libraries' `float` maths differs in the last digit between Windows' UCRT and glibc.

### 13.6 The rule for changes

A change to start-point, lap or segment logic needs the default run on seed sets 0 to 3 **and** the 5,000-run sweeps on all four, each compared with a "before" binary built from the previous commit on the same test tree. A default run alone can pass a change that breaks other draws; comparing run by run on the same traces shows exactly what moved.

## 14. Known limits

From the synthetic tests. Noise figures are white noise per axis at HDOP 1.

**Counting mode**

- **A dropout that hides both the far side and the gate passage**, with GPS distance stopped too, loses the lap: nothing tells counting mode one went by. If the kernel keeps counting distance through the gap, the lap is credited at the next crossing.
- **GPS distance that grows while standing still** never credits a lap, but after a long rest mid-run the part-lap estimate can sit at the 98% cap until the next crossing.
- **The lane-change formula** uses the lap fraction. Physically, moving out on the back straight adds only the remaining bend's extra length: for lane 1 to 8 at half a lap the two differ by about 11 m on that lap. Calibrated mode credits it physically.
- **Lap 1 progress** shows 0 until the runner is about 20 m from the start point, then jumps.
- **The GPS track moving more than 35 m** from the start point mid-run puts passes outside `passRadiusM`; laps are then only credited after reading gaps.

**Calibration and calibrated mode**

- **R within 0.5 m at lock** is not reached above about 2.5 m noise (section 13.5). R rests on where the straights end. It matters little: 0.75 m of R error adds about 0.3 m to the part-lap error.
- **The centre is only as good as the GPS over the calibration laps**: with drift, up to 5.9 m from the true track in the tests. Drift tracking then follows the trace, so distances are unaffected.
- **Part-lap within 5 m (95th percentile) at 3 m noise** is not reached: 5.86 m from scratch, 5.14 m with a stored model. What is left is drift along the track.
- **Resting just short of the line.** A runner who stops a few metres before the line, within the noise, can have the lap counted on arrival instead of when they set off again: the count is right, the lap time includes the rest. Easing up past the line is timed correctly.
- **A lane change set on a bend** is counted on the new line from the moment it is set: up to 3.8 m over for lane 1 to 8 while the runner moves across.
- **A short cut across the infield** (under 30 s off the track) is credited as far as the runner's speed would have taken them on the track, so as not to lose distance to a false alarm.
- **The display can hold at the mid-run switch** while the re-measured lap catches up: up to 43 m in 10,000 runs.
- **A long spell of HDOP over 4** holds the display, as a dropout does: calibrated mode moves only on good readings. Up to 247 m behind at the end of a 60 s spell; laps and the total are right once readings return.
- **A lap credited across a gap** is timed by interpolation and not refined: 6.4 s out at worst in the spell tests.

**Stored tracks**

- **A day's offset in the undecided band** (3.5 to 5.5 standard errors) is applied in part while lining up; drift tracking finishes it at 1 m a lap.
- **A right track dropped as a wrong pick**: 0 to 1 in 5,000 runs a seed set, at the noisy end. Its laps are still counted by counting mode.
- **A turned neighbour dropped after laps were shown** cannot take back a lap the wrong geometry counted and counting mode did not: 0 to 2 in 300 runs at 40 m, 0 or 1 at 60 m.

**Road and track**

- **A join undone**, at 4 to 5 m noise or with a day's offset of 12 to 20 m: the service sees a `SegmentChanged` to Track and back. About 1 run in 5,000 at offsets up to 5 m, 1 in 1,250 at up to 20 m. Laps and distances end up right.
- **With a day's offset of 14 to 20 m**, lining up mid-run can stop with part of the offset applied; the first lap after the join is then closed inexactly by an exit and rejoin. About 4 runs in 10,000.
- **A join is known late:** about 7 s at the median after the runner comes onto a stored track, 33 s at the 95th percentile, longer with a large day's offset. Events are timed at the entry all the same.
- **Lap 1 at a join**: 95th percentile 8.0 m on a stored track and 10.2 m for a lane the runner sets, at 2 to 4 m noise. At 4 m noise the 4 to 6 approach readings in the ring place a stored track's entry no closer than 6 to 8 m. On a stored track, shallow approaches (30 m back, 8 m out) and runs along the lane give no clear slant, and lap 1 is credited +26 to +41 m on average: laps count from where the runner came onto the line.
- **A stored lane far from the runner's.** A join noticed takes the lane last run there. A runner on another lane stays on the track (laps counted, each its lane length, distance on the lane set) until they set their lane; their leaving is placed at the first reading off, about 9 s late in the tests (3 s with the right lane set), since no reading is close to the lane set's line.
- **A lane set on the road** joins at the latest good reading: lap 1 starts with that reading's noise (95th percentile 10.2 m at 2 to 4 m noise).
- **Walking on or off a track** at 1.5 m/s, where GPS distance is mostly jitter, costs 10 to 30 m for a few seconds' placement of the transition.
- **The replay's cost falls in one reading**: a join the Session notices replays the readings since the runner came onto the track, with the calibrator's fits. On the PC the worst replay took 67 us and the worst ordinary reading 165 us (Release).

**Starts**

- **A walker at 1.0 to 1.5 m/s** is treated as standing, so lap 1 credits a few metres more than was walked (section 11).
- **A lane at Start off the track** (in a car park beside it) starts counting mode there: it measures loops round the start until the runner is `exitCountingM` (250 m) away, when the visit ends. Laps on the track need the lane set there.

**The noisy end**

- **Noise beyond about 5 m an axis** (4.4 to 5 m at HDOP up to 1.5), or day offsets of 10 to 20 m that drift tracking cannot follow, gives an inexact or missing lap in about 1 run in 5,000.
- **`uereM` and the link between HDOP and noise** are assumptions of the app and of the generator (noise is the noise figure times HDOP); recorded traces in format version 2 carry the HDOP to check them.
- **Version 1 traces** are converted assuming their figure was metres.

## 15. Memory and stack

ARM `sizeof` (GCC for Cortex-M33, `-Os`):

| Class | Bytes |
|---|---|
| `Session` (everything) | 11,560 |
| of which `Tracker` | 6,192 |
| of which the ring (376 points of 12 bytes) | 4,512 |
| of which the rest (odometer, frame, lane table, visited tracks, join state, the latest good reading) | 856 |
| `Tracker`: `Calibrator` (3,072 of it the 512 points) | 3,576 |
| `Tracker`: `LapCounter` | 1,096 |
| `Tracker`: `LapTiming` | 484 |
| `Config` (held by `LapCounter`, `Calibrator` and `Tracker`) | 292 |
| `TrackModel` | 48 |
| `TrackStore` | 568 |
| `TrackRecord`, `StoreOutcome` | 56, 64 |
| `Session::Events` (returned by value), `Session::Event` | 708, 44 |
| `Tracker::Events`, `Tracker::Event` | 196, 24 |

Everything is static or a member; nothing large is on the stack.

**Stack** (`-fstack-usage`). The deepest chain is a reading that joins a stored track, whose replay runs a calibrator fit: `Session::update` 72, `roadStep` 96, `nearStep` 472, `joinKnown` 96, `replay` 360, `Tracker::update` 168, `Calibrator::add` 56, `runFit` 144, `fit` 536, `residualScale` 360, `evaluate` 56: about 2.4 KB, plus the 708-byte `Session::Events` in the caller's frame. `entryAt` (536 bytes) runs before the replay, not under it. An ordinary reading on a calibrated track runs through `trackStep` and `lineReading`. The service's stack budget and its measured deepest chain are in [README.md](../README.md).
