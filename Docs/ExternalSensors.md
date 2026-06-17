# External Sensors (BLE Accessories)

The watch can acquire an **external BLE sensor** — for v1, a heart-rate chest
strap or arm band — and feed its readings into the normal sensor pipeline for
the duration of an activity. This page describes the app-facing contract.

## Opt-in: nothing happens unless an app asks

External-sensor acquisition is **explicitly opt-in per app**. An app that never
sends `RequestPrepare` never triggers scanning and behaves exactly as before.
There is no implicit/background strap connection — acquisition is scoped to a
workout.

Heart-rate data continues to arrive through the existing `HEART_RATE` sensor
type; you do **not** change how you read HR. The accessory messages only drive
*acquisition* and surface *status*.

## Messages

`SDK/Messages/AccessoryMessages.hpp`:

| Message | Direction | Payload |
|---|---|---|
| `RequestPrepare`  | app → kernel | `kinds` (bitmask of `SDK::Accessory::Kind`) |
| `RequestRelease`  | app → kernel | `kinds` (`0` = release all) |
| `EventStatus`     | kernel → app | `kind`, `state` (`SDK::Accessory::State`), `name[24]` |

`Kind` is a bitmask so a single prepare can request several accessory kinds;
v1 defines `HRM = 1<<0` and reserves the remaining bits for future cadence /
power profiles.

### Status flow

```
        RequestPrepare(HRM)
IDLE ─────────────────────────▶ SEARCHING ──▶ CONNECTING ──▶ CONNECTED
  ▲                                  │                            │
  │ RequestRelease / app stop        │ no strap / feature off     │ link drop
  │                                  ▼                            ▼
  └───────────────────────────── UNAVAILABLE                    LOST ──▶ (re-acquire)
```

`UNAVAILABLE` means the feature is disabled or no accessory is remembered —
treat it as "external HR not coming". `LOST` is reported when a connected strap
drops; the kernel falls back to the optical sensor and tries to re-acquire.

## Recommended app usage

- Send `RequestPrepare(HRM)` when the **pre-activity screen** appears, and tell
  the user to put the strap on / wake it before starting. Acquisition runs while
  they get ready.
- Subscribe to `EventStatus` and show a strap indicator (searching / connected /
  lost) so the user knows whether external HR is live.
- Send `RequestRelease` when leaving the activity. The kernel also releases
  automatically if the app stops, so a crash can never leave a strap connected.

## Reading the HR source (optional)

`SensorDataParserHeartRate` exposes `getSource()` →
`Source::{UNKNOWN, OPTICAL, EXTERNAL}`. Newer kernels tag each `HEART_RATE`
sample with its source; older kernels emit only BPM + trust, for which
`getSource()` returns `UNKNOWN`. The parser tolerates both layouts, so an app
built against this SDK works on older firmware and vice versa. Use the source to
label records (e.g. a FIT `hr_source` developer field) — do not gate HR display
on it.
