# BeatProbe

A diagnostic wired into the Sensors tutorial. It answers one question on real hardware: does `HEART_BEAT`
(0x40) deliver timed peak events (so RR intervals are just the gaps between beat timestamps), or is only the
raw `PPG` (0xF0) waveform available? It logs the beat/PPG timing and prints a verdict.

This branch is pre-wired. Build the Sensors app, flash it, wear the watch, read the log.

## Build

```bash
export UNA_SDK=/path/to/una-sdk
cd "$UNA_SDK/Docs/Tutorials/Sensors/Software/Apps/Sensors-CMake"
cmake -S . -B build
cmake --build build
# produces Docs/Tutorials/Sensors/Output/Sensors_<version>.uapp
```

## Flash (per `Docs/deploy.md`)

Connect by USB, copy the `.uapp` into an `Apps/Sensors/` folder on the watch, eject, power-cycle, launch it.

## Run

Wear the watch (worn and reasonably still) for ~90 s after launch, then read the debug UART:

```bash
grep -E "PROBE|BEAT-EVENT|^beat|^ppg" <uart-capture.log>
```

From launch it logs an `armed` banner, a raw `beat #...` line per beat (timestamp + inter-beat `dt` in ms) or
a `ppg ...` line each second, then a summary block every 15 s, at 90 s, and on exit:

```
==== UNA BEAT-EVENT PROBE armed: window=90s, report every 15s, watching 0x40/0xF0/0x41/0x140 ====
beat #1   t=10240 ms (+512 us)  dt=0 ms     fields=1  f0=0x00000000
beat #2   t=11051 ms (+128 us)  dt=811 ms   fields=1  f0=0x00000000
...
==== UNA BEAT-EVENT PROBE [FINAL ] t=90s ==========================
  HEART_BEAT 0x40 : events=92  rate=61/min  fields=1  field0=0x00000000
  RR intervals    : n=90  min/mean/max=812/986/1170 ms  last=995 ms  RMSSD~34 ms  rejected=1  subMs_ts=88/92
  PPG 0xF0        : events=0  samples=0  est_rate=0 Hz  channels=0
  cross-check     : HEART_RATE 0x41 present (bpm=60 trust=98)  impliedHR(fromRR)=60  TOUCH worn=yes
  VERDICT A : ...
```

`VERDICT A` = beats give RR directly; `B` = only PPG (RR needs peak detection); `C` = no cardio data (check
worn + HR calibration). The raw `beat`/`ppg` lines are the evidence; the summary and verdict are derived from
them, so the log is conclusive even if the run is short or the capture is truncated.

## What to send back

The full captured log from the `armed` banner onward. If no cardio data appears, note whether the watch was
worn and whether BLE HR calibration had run.
