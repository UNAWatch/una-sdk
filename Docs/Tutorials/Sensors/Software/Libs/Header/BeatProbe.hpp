/**
 ******************************************************************************
 * @file    BeatProbe.hpp
 * @brief   Drop-in diagnostic that answers the "beat-event question":
 *          does HEART_BEAT (0x40) deliver timed peak events (=> HRV is cheap),
 *          or must RR be recovered from the raw PPG (0xF0) waveform
 *          (=> HRV needs an on-watch peak detector)?
 *
 * It observes four streams that the Sensors tutorial already subscribes to:
 *   - HEART_BEAT     (0x40)  beat peak events  -> RR = delta of timestamps
 *   - PPG            (0xF0)  raw optical waveform
 *   - HEART_RATE     (0x41)  BPM + trust       -> cross-check vs RR-derived HR
 *   - TOUCH_DETECT   (0x140) worn / unworn      -> gates the "no data" case
 *
 * No GUI, no filesystem, no FIT. Output is log-only and self-interpreting. It logs a
 * raw line per beat (timestamp + inter-beat interval) and ~1 PPG line/s as it goes, plus
 * a summary block every 15 s, one at 90 s, and one on exit, each ending in VERDICT A/B/C.
 * Even a truncated or early-terminated log is conclusive. Grep for "PROBE" or "BEAT-EVENT".
 *
 * Integer math only (no <cmath>, no float printf) so it is robust on the
 * MCU's newlib. Single-instance: declare one file-static BeatProbe.
 ******************************************************************************
 */
#pragma once

#include <cstddef>   // SensorDataBatch.hpp uses size_t without including this itself
#include <cstdint>

#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/UnaLogger/Logger.h"

// Route every probe line through the kernel logger under module tag "PROBE".
// Defined here and #undef'd after the class so nothing leaks into the TU.
#define BP_LOG(...) Logger_message("I", "PROBE", __func__, __LINE__, __VA_ARGS__)

class BeatProbe
{
public:
    /// HEART_BEAT (0x40): one (or a batch of) beat peak event(s).
    void onBeat(const SDK::Sensor::DataBatch& b)
    {
        for (uint16_t i = 0; i < b.size(); ++i) {
            const SDK::Sensor::DataView v      = b[i];
            const uint64_t              tUs    = v.getTimestampUs();
            const bool                  haveRr = (mHaveLastBeat && tUs > mLastBeatUs);
            const uint32_t              dtMs   = haveRr ? static_cast<uint32_t>((tUs - mLastBeatUs) / 1000ULL) : 0u;

            mBeatCount++;
            mBeatFields = v.getFieldCount();
            if (v.getFieldCount() >= 1) mBeatField0 = v.u[0];
            if ((tUs % 1000ULL) != 0ULL) mBeatSubMs++;   // sub-ms timestamp present?

            if (haveRr) {
                mRrLastMs = dtMs;
                if (dtMs >= kRrMinMs && dtMs <= kRrMaxMs) {     // plausible single-beat RR
                    mRrSumMs += dtMs;
                    mRrN++;
                    if (dtMs < mRrMinObs) mRrMinObs = dtMs;
                    if (dtMs > mRrMaxObs) mRrMaxObs = dtMs;
                    if (mHavePrevRr) {                          // RMSSD: successive differences
                        const int32_t d = static_cast<int32_t>(dtMs) - static_cast<int32_t>(mPrevRrMs);
                        mSsdSum += static_cast<uint64_t>(static_cast<int64_t>(d) * d);
                        mSsdN++;
                    }
                    mPrevRrMs   = dtMs;
                    mHavePrevRr = true;
                } else {
                    mRrOut++;
                    mHavePrevRr = false;                        // artifact breaks the diff chain
                }
            }

            // Raw evidence, one line per beat, so the answer survives even if no report
            // block does. dt is the inter-beat (RR) interval in ms; 0 on the first beat.
            BP_LOG("beat #%u  t=%u ms (+%u us)  dt=%u ms  fields=%u  f0=0x%08x\n",
                   mBeatCount, v.getTimestamp(), static_cast<uint32_t>(tUs % 1000ULL),
                   dtMs, v.getFieldCount(), (v.getFieldCount() >= 1 ? v.u[0] : 0u));

            mLastBeatUs   = tUs;
            mHaveLastBeat = true;
        }
    }

    /// PPG (0xF0): raw waveform; measure effective sample rate and channel count.
    void onPpg(const SDK::Sensor::DataBatch& b)
    {
        const uint16_t n = b.size();
        if (n == 0) return;
        mPpgEvents++;
        mPpgSamples += n;
        mPpgFields = b[0].getFieldCount();
        const uint64_t firstUs = b[0].getTimestampUs();
        const uint64_t lastUs  = b[n - 1].getTimestampUs();
        if (!mPpgHaveFirst) { mPpgFirstUs = firstUs; mPpgHaveFirst = true; }
        if (lastUs > mPpgLastUs) mPpgLastUs = lastUs;

        // Raw evidence, throttled to ~1 line/s, proving the waveform actually streams.
        const uint32_t nowMs = static_cast<uint32_t>(mPpgLastUs / 1000ULL);
        if (nowMs - mLastPpgLogMs >= 1000u) {
            mLastPpgLogMs = nowMs;
            BP_LOG("ppg  t=%u ms  total_samples=%u (+%u)  channels=%u\n", nowMs, mPpgSamples, n, mPpgFields);
        }
    }

    /// HEART_RATE (0x41): processed BPM + trust, used only as a sanity cross-check.
    void onHr(const SDK::Sensor::DataBatch& b)
    {
        if (b.size() == 0) return;
        const SDK::Sensor::DataView v = b[0];
        if (v.getFieldCount() >= 2) { mHrBpm = v.f[0]; mHrTrust = v.f[1]; mHaveHr = true; }
    }

    /// TOUCH_DETECT (0x140): worn / unworn, to distinguish "off wrist" from "no signal".
    void onTouch(const SDK::Sensor::DataBatch& b)
    {
        if (b.size() == 0) return;
        const SDK::Sensor::DataView v = b[0];
        if (v.getFieldCount() >= 1) mWorn = (v.u[0] != 0u) ? 1 : 0;
    }

    /// Call once per service-loop iteration; self-throttles its own reporting.
    void tick(uint32_t nowMs)
    {
        if (!mStarted) {
            mStartMs      = nowMs;
            mLastReportMs = nowMs;
            mStarted      = true;
            BP_LOG("==== UNA BEAT-EVENT PROBE armed: window=%us, report every %us, watching 0x40/0xF0/0x41/0x140 ====\n",
                   kWindowMs / 1000u, kReportEveryMs / 1000u);
            return;
        }
        const uint32_t elapsedMs   = nowMs - mStartMs;
        const bool     interimDue  = (nowMs - mLastReportMs) >= kReportEveryMs;
        const bool     finalDue    = (!mPrintedFinal) && (elapsedMs >= kWindowMs);
        if (!interimDue && !finalDue) return;
        mLastReportMs = nowMs;
        report(elapsedMs, finalDue);
        if (finalDue) mPrintedFinal = true;
    }

    /// Emit a final block immediately, e.g. when the app is stopping, so whatever was
    /// accumulated is captured even if the 90 s window was never reached.
    void flushFinal(uint32_t nowMs)
    {
        if (!mStarted) return;
        report(nowMs - mStartMs, true);
        mPrintedFinal = true;
    }

private:
    // Integer square root (RMSSD is reported in whole ms; avoids the float lib).
    static uint32_t isqrt(uint64_t x)
    {
        uint64_t res = 0;
        uint64_t bit = 1ULL << 62;
        while (bit > x) bit >>= 2;
        while (bit != 0) {
            if (x >= res + bit) { x -= res + bit; res = (res >> 1) + bit; }
            else                  res >>= 1;
            bit >>= 2;
        }
        return static_cast<uint32_t>(res);
    }

    void report(uint32_t elapsedMs, bool isFinal)
    {
        uint32_t sec = elapsedMs / 1000u;
        if (sec == 0) sec = 1;
        const uint32_t beatRate  = (mBeatCount * 60u) / sec;                       // beats/min
        const uint32_t meanRr    = (mRrN > 0) ? static_cast<uint32_t>(mRrSumMs / mRrN) : 0u;
        const uint32_t impliedHr = (meanRr > 0) ? (60000u / meanRr) : 0u;          // bpm from RR
        const uint32_t rmssd     = (mSsdN > 0) ? isqrt(mSsdSum / mSsdN) : 0u;

        uint32_t ppgHz = 0;
        if (mPpgHaveFirst && mPpgLastUs > mPpgFirstUs && mPpgSamples > 1) {
            const uint64_t spanUs = mPpgLastUs - mPpgFirstUs;
            ppgHz = static_cast<uint32_t>((static_cast<uint64_t>(mPpgSamples - 1) * 1000000ULL) / spanUs);
        }

        const char* tag = isFinal ? "FINAL " : "interim";
        BP_LOG("==== UNA BEAT-EVENT PROBE [%s] t=%us ==========================\n", tag, sec);
        BP_LOG("  HEART_BEAT 0x40 : events=%u  rate=%u/min  fields=%u  field0=0x%08x\n",
               mBeatCount, beatRate, mBeatFields, mBeatField0);
        BP_LOG("  RR intervals    : n=%u  min/mean/max=%u/%u/%u ms  last=%u ms  RMSSD~%u ms  rejected=%u  subMs_ts=%u/%u\n",
               mRrN, (mRrN ? mRrMinObs : 0u), meanRr, mRrMaxObs, mRrLastMs, rmssd, mRrOut, mBeatSubMs, mBeatCount);
        BP_LOG("  PPG 0xF0        : events=%u  samples=%u  est_rate=%u Hz  channels=%u\n",
               mPpgEvents, mPpgSamples, ppgHz, mPpgFields);
        BP_LOG("  cross-check     : HEART_RATE 0x41 %s (bpm=%d trust=%d)  impliedHR(fromRR)=%u  TOUCH worn=%s\n",
               mHaveHr ? "present" : "ABSENT", static_cast<int>(mHrBpm), static_cast<int>(mHrTrust),
               impliedHr, (mWorn < 0 ? "?" : (mWorn ? "yes" : "no")));

        const bool beatsViable = (mRrN >= kMinRrForVerdict) &&
                                 (beatRate >= 30u && beatRate <= 220u) &&
                                 (meanRr  >= 300u && meanRr  <= 1800u);
        const bool ppgViable   = (ppgHz >= kMinPpgHz) || (mPpgSamples >= kMinPpgSamples);

        if (beatsViable) {
            BP_LOG("  VERDICT A : HEART_BEAT(0x40) DELIVERS TIMED PEAK EVENTS. RR is derivable directly\n");
            BP_LOG("              (mean %u ms ~ %u bpm, RMSSD ~%u ms, %s timestamps). => on-watch HRV is an\n",
                   meanRr, impliedHr, rmssd, (mBeatSubMs > 0 ? "sub-ms" : "1-ms"));
            BP_LOG("              S-M streaming-stats task on 0x40; NO on-watch PPG peak detection needed.\n");
        } else if (ppgViable) {
            BP_LOG("  VERDICT B : No usable HEART_BEAT events, but PPG(0xF0) streams (~%u Hz, %u channel(s)).\n",
                   ppgHz, mPpgFields);
            BP_LOG("              RR must be recovered by peak detection on the PPG waveform. => on-watch HRV\n");
            BP_LOG("              is an L signal-processing task (fixed-point detector + motion rejection).\n");
        } else {
            BP_LOG("  VERDICT C : No cardio data yet (beats=%u, PPG samples=%u). Confirm watch is WORN (worn=%s)\n",
                   mBeatCount, mPpgSamples, (mWorn < 0 ? "?" : (mWorn ? "yes" : "no")));
            BP_LOG("              and that BLE HR sensor calibration has run (the Service logs a calibration note),\n");
            BP_LOG("              then re-run worn and still for the full window.\n");
        }
        BP_LOG("===============================================================\n");
    }

    // --- window / cadence ---
    bool     mStarted       = false;
    bool     mPrintedFinal  = false;
    uint32_t mStartMs       = 0;
    uint32_t mLastReportMs  = 0;

    // --- HEART_BEAT (0x40) ---
    uint32_t mBeatCount  = 0;
    uint32_t mBeatFields = 0;
    uint32_t mBeatField0 = 0;
    uint32_t mBeatSubMs  = 0;
    bool     mHaveLastBeat = false;
    uint64_t mLastBeatUs   = 0;

    // --- RR / RMSSD ---
    uint32_t mRrN      = 0;
    uint32_t mRrOut    = 0;
    uint64_t mRrSumMs  = 0;
    uint32_t mRrMinObs = 0xFFFFFFFFu;
    uint32_t mRrMaxObs = 0;
    uint32_t mRrLastMs = 0;
    bool     mHavePrevRr = false;
    uint32_t mPrevRrMs   = 0;
    uint64_t mSsdSum     = 0;
    uint32_t mSsdN       = 0;

    // --- PPG (0xF0) ---
    uint32_t mPpgEvents    = 0;
    uint32_t mPpgSamples   = 0;
    uint32_t mPpgFields    = 0;
    uint32_t mLastPpgLogMs = 0;
    bool     mPpgHaveFirst = false;
    uint64_t mPpgFirstUs   = 0;
    uint64_t mPpgLastUs    = 0;

    // --- cross-checks ---
    bool  mHaveHr  = false;
    float mHrBpm   = 0.0f;
    float mHrTrust = 0.0f;
    int   mWorn    = -1;   // -1 unknown, 0 no, 1 yes

    // --- tunables ---
    static constexpr uint32_t kWindowMs        = 90000;  // print a FINAL verdict at 90 s
    static constexpr uint32_t kReportEveryMs   = 15000;  // interim progress every 15 s
    static constexpr uint32_t kRrMinMs         = 250;    // accept RR in [250, 2500] ms (HR 24..240)
    static constexpr uint32_t kRrMaxMs         = 2500;
    static constexpr uint32_t kMinRrForVerdict = 20;     // need >=20 valid RR to trust VERDICT A
    static constexpr uint32_t kMinPpgHz        = 10;     // PPG counts as a usable stream at >=10 Hz
    static constexpr uint32_t kMinPpgSamples   = 200;
};

#undef BP_LOG
