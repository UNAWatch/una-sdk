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

## Beat-to-beat R-R intervals (`RR_INTERVAL`, experimental)

> **Experimental — there is no firmware producer yet.** `0x44` is the SDK-side
> half of the contract, landed so HRV / DFA-alpha1 work can be built against a
> fixed shape. Do not ship against it until a producer exists.

`HEART_RATE` and `HEART_RATE_EX` both carry a *rate*. HRV metrics (RMSSD, SDNN,
DFA-alpha1, threshold detection) need the individual beat-to-beat intervals
instead, which a strap reports in the same `0x2A37` notification as the BPM.
`RR_INTERVAL` is the opt-in sensor type that carries them, parsed with
`SDK::SensorDataParser::RrInterval`:

- `getRrMs()` — the interval in milliseconds;
- `getBpm()` — the implied instantaneous rate, a convenience, **not** a smoothed
  HR;
- `getSource()` → `Source::{UNKNOWN, OPTICAL, EXTERNAL, ECG}` — the shared
  values are `static_assert`ed against `HeartRateEx::Source`;
- `hasDiscontinuity()` / `isArtifactSuspect()` / `isSkinContactLost()` — quality
  and continuity signals;
- `checkContinuity(prevTimestampUs)` → `Continuity::{UNUSABLE, CONTIGUOUS, GAP,
  REORDERED}` — whether this interval continues from the previous one. A lost or
  reordered frame is otherwise undetectable.

**This page owns the producer half; `SensorDataParserRrInterval.hpp` owns the
consumer half.** Read both before writing either. The rules most easily assumed
wrongly: one interval per frame rather than one per callback; mixed field types
rather than the all-float `HEART_RATE_EX` layout; a frame timestamp that is a beat
instant rather than an arrival; event-based delivery; and which frame counts as
"previous" when checking continuity — the consumer's obligation, and not
simply the frame you saw last.

### Notes for whoever writes the producer

The R-R array is the **last** field of the `0x2A37` notification and its offset is
variable, so a decoder has to walk the flags byte rather than assume a layout:

| Flags bit | Effect on the layout |
|---|---|
| 0 | HR value is `uint8` (0) or `uint16` (1) |
| 1 | sensor contact detected — the source for `NO_SKIN_CONTACT` |
| 2 | sensor contact supported; when 0, bit 1 carries no information |
| 3 | a 2-byte *energy expended* field is present, **before** the R-R array |
| 4 | the R-R array is present |

The R-R values themselves are `uint16` in units of 1/1024 s, oldest first, filling
the remainder of the notification.

Bit 3 is the trap. The kernel decoder reads only the 16-bit-value,
contact-detected and contact-supported bits (Ross Ryles, PR #220), so a producer
extended from it will have every R-R offset wrong on any strap that reports energy
expended — and the misread values still look like plausible intervals, so nothing
downstream will flag it.

`NO_SKIN_CONTACT` collapses bits 1 and 2 into one flag, which fails in the safe
direction: 0 means "do not discard", which is also what "contact detection
unsupported" and "flags not populated" mean. If a producer needs to distinguish
them, a new flag bit can be added at any time without breaking anything already
built — undefined bits are ignored by the accessors.

Leave `DETECTOR_STAMPED` clear: decoding `0x2A37` means seeing a notification
arrive, not a beat, so the claim that flag makes does not apply to you. No budget
turns on it today — there is one continuity budget, because nothing in this SDK can
size a tighter one — but it is the record of how the stamp was obtained, a consumer
may key its own budget on it, and it is the one bit that could later make a consumer
stricter rather than more forgiving. Clearing it is always safe.

### What a producer owes

None of this is enforced by the SDK.

- **Register `getFieldsNumber()` as the delivery stride**, not the parse minimum.
  The stride is fixed per driver and a consumer's field count derives from it, so a
  narrower registration makes the later fields permanently undeliverable.
- **Write every field you registered, on every frame.** Nothing guarantees a frame
  is cleared between pushes, so an unwritten field may still hold the previous
  one's value. Set `DISCONTINUITY` after a reconnect and never rewrite `FLAGS`, and
  the bit stays set for the session — which reads downstream as a stream that is
  never contiguous, and makes an HRV consumer discard every window.
- **A producer that knows it has a gap says so with `DISCONTINUITY`.**
- **Emit a frame only when you have an interval.** There are no status-only frames:
  `isDataValid()` rejects a zero interval and every accessor that reads the frame
  then reads it as absent, so its flags would not arrive anyway.
- **Report float milliseconds** whatever the native unit. A strap's 1/1024 s ticks
  convert losslessly: one tick is 125/128 ms, exact in binary floating point, and
  `n * 125` stays under float32's 2^24 exact-integer limit for every count a 16-bit
  wire field can hold.
- **Register event-based delivery** (`Driver::Mode::EVENT_BASED` on the simulator's
  driver; on-device, the kernel's equivalent). Nothing guarantees a periodic path
  preserves every frame, and a decimated beat stream loses beats rather than
  coarsening.
- **Register a minimum delivery period no longer than the shortest interval you
  will emit**, and do so even where you also register the mode above. A
  rate-adapting path is free to thin frames arriving closer together than the period
  it was given; thinning a beat stream is losing beats, and nothing guarantees a
  given platform's event path bypasses rate adaptation.
- **Stamp the beat instant the interval ends on, in the microsecond domain.** An
  interval is fractional in milliseconds, so whole-millisecond stamping throws that
  fraction away for nothing. Put whole milliseconds in `mTimeStamp` and the
  sub-millisecond remainder in `mTimeStampUs`.

If your detector reports beat instants, stamp those and set `DETECTOR_STAMPED`. If
you see only arrival, reconstruct them — for the `k` intervals of a notification
arriving at `T`, in wire order, `t[i] = T - SUM(rr[j] for j > i)` — and leave the
bit clear. That subtraction is a recipe for a producer with nothing better, not the
definition of the field. Where it underflows, because a notification arrives less
than `SUM(rr)` after boot, emit only the intervals whose instant is representable
and set `DISCONTINUITY` on the first emitted. Do not invent a stamp for the rest,
and never stamp zero, which is not an instant.

Strap battery level is out of scope: it lives on a separate standard service
(`0x180F`), so it is neither the same packet nor HRV-relevant.
