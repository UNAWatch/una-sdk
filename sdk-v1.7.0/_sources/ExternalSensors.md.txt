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

`HEART_RATE` is unchanged — it stays the stable 2-field (BPM, trust) frame, and
`SensorDataParserHeartRate` reads it exactly as before. Source provenance is
exposed separately, opt-in, via the `HEART_RATE_EX` sensor type so existing
consumers keep working untouched.

Subscribe to `HEART_RATE_EX` and parse it with `SensorDataParserHeartRateEx`,
which exposes:

- `getSource()` → `Source::{UNKNOWN, OPTICAL, EXTERNAL}` — which source produced
  the arbitrated reading;
- `getBpm()` / `getTrustLevel()` — the arbitrated value (same as `HEART_RATE`);
- `getOpticalBpm()` / `getOpticalTrust()` and `getExternalBpm()` /
  `getExternalTrust()` — the raw per-source readings (0 when absent/stale).

Validation is lenient (a future kernel may append fields), so an app built
against this SDK keeps working as the frame grows. Use this to label records or
log separate FIT series (e.g. `hr_source`, internal-PPG, and external-strap HR)
— do not gate HR display on it. Apps that only need BPM should stay on
`HEART_RATE` and ignore `HEART_RATE_EX` entirely.
