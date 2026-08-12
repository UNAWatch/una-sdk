/**
 ******************************************************************************
 * @file    SensorDataParserRrInterval.hpp
 * @brief   SensorData parser for the RR_INTERVAL (beat-to-beat) sensor.
 ******************************************************************************
 *
 * EXPERIMENTAL. There is no firmware producer yet, and 0x44 is not stable until
 * one has been written against this header.
 *
 * One beat-to-beat R-R interval per frame — the gap between consecutive
 * heartbeats, which is what RMSSD, SDNN and DFA alpha1 are computed from. Opt-in:
 * offering it must not change what a HEART_RATE consumer already sees.
 *
 * FIELD LAYOUT. Positional, each field a fixed type in the Data union — unlike
 * HEART_RATE_EX this frame is not all-float, so a producer writes through the
 * matching writer (.f / .u) or a consumer reads garbage.
 *
 *   [0] rr_ms   float, REQUIRED — the interval in milliseconds
 *   [1] source  u32   — Source
 *   [2] flags   u32   — Flag bits
 *   [3] RESERVED for a graded per-beat confidence. Nothing else may take it.
 *
 * Data carries no in-band length field, so a frame cannot hold a variable-length
 * burst: the several intervals of one 0x2A37 notification arrive as consecutive
 * one-interval frames, and a consumer iterates DataBatch::size() rather than
 * assuming one frame per callback.
 *
 * WHAT A PRODUCER OWES is in Docs/ExternalSensors.md, beside the 0x2A37 decode a
 * producer has to perform anyway. Why the budget below is the size it is, and
 * which values are still open, are in the pull request that introduced this file.
 *
 * TIMESTAMP. The frame stamp is the beat instant the interval ends on. That is
 * the invariant, not how a producer reaches it: a detector reporting beat instants
 * stamps them and sets DETECTOR_STAMPED, one seeing only arrival reconstructs them
 * and leaves the bit clear. Unlike a field this cannot be tightened later, because
 * a consumer relying on true beat instants breaks the moment a producer stops
 * supplying them.
 *
 * The stamp is TWO FIELDS AND THE SECOND IS A REMAINDER. getTimestampUs() forms
 * ms * 1000 + us, a duration only while us stays under 1000, so whole milliseconds
 * go in mTimeStamp and sub-millisecond microseconds in mTimeStampUs, as
 * SensorDataSample::setTimestampUs writes them. Used as a full counter the field
 * doubles the apparent interval. checkContinuity() answers UNUSABLE for a frame it
 * can see breaking this, which is a backstop and not a guarantee: it cannot inspect
 * prevUs, a full counter landing under 1000 by chance still gets a verdict, and a
 * frame declaring DISCONTINUITY is answered before the check is reached.
 *
 * CONTINUITY. rr_ms is authoritative for the interval VALUE, the timestamp for
 * CONTINUITY, so a disagreement between t[n] - t[n-1] and rr[n] is a lost or
 * reordered frame and nothing else detects one — there is no sequence number.
 * checkContinuity() IS that check. Do not compare the stamps for equality; the
 * budget is what the verdict rests on.
 *
 * DISCONTINUITY outranks the stamps and is answered before them. Zero is "no
 * stamp" at either end.
 *
 * The budget is a WINDOW, not a bound. A slow connection interval, a link-layer
 * retransmission or a drain stall each produce an honest comparison reading GAP,
 * and nothing here rules them out: read an isolated GAP as one gap that cannot be
 * accounted for, not as a corrupt stream. A consumer holding a real figure passes
 * it to the two-argument overload. Erring loose is deliberate — the expensive
 * error is the false positive, which discards the windows the strap was connected
 * for.
 *
 * Two accepted residuals. A lost beat goes undetected once
 * the whole missing interval fits inside the budget, which is a rate above
 * kGuaranteedDetectableBpm; and Data's millisecond field is uint32, so the single
 * pair straddling its wrap reads REORDERED.
 *
 * CONSUMER OBLIGATION. The stamp you pass is the previous frame whose
 * isDataValid() HELD — whatever verdict that frame got — and zero until you have
 * seen one. Nothing checks it, and both ways of getting it wrong are silent.
 * Advance across an invalid frame and you carry its zero forward, so the next
 * pair reads UNUSABLE where a beat's interval in fact went unreported, which is
 * a GAP and a window to discard. Advance only on CONTIGUOUS
 * and nothing re-establishes the stream after a reordering.
 *
 ******************************************************************************
 */
#ifndef SDK_SENSORLAYER_DATAPARSERS_SENSOR_DATA_PARSER_RR_INTERVAL_HPP
#define SDK_SENSORLAYER_DATAPARSERS_SENSOR_DATA_PARSER_RR_INTERVAL_HPP

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cmath>
#include <cstdint>
#include <type_traits>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class for parsing one RR_INTERVAL frame from a DataView.
         */
        class RrInterval
        {
        public:
            /**
             * @brief Field indices. Only RR_MS is required for a frame to parse;
             *        a producer still registers COUNT as its delivery stride.
             * @note  The indices are frozen: arbitrary when picked, fixed now.
             */
            enum Field : uint8_t {
                RR_MS  = 0, ///< R-R interval, milliseconds, float (required)
                SOURCE = 1, ///< producer of this interval, u32 (0 = not reported)
                FLAGS  = 2, ///< quality / continuity bits, u32 (0 = not reported)
                COUNT  = 3  ///< Field count (3)
            };

            /**
             * @brief Which producer emitted the interval.
             * @note  Mirrors HeartRateEx::Source, extended with ECG. The shared
             *        values are locked by static_assert below to HeartRateEx and
             *        through it to the kernel HR arbiter enum, which defines them.
             * @note  ECG is convention: the arbiter has no value for it, so no
             *        static_assert holds it. Settle it before anything ships.
             */
            enum class Source : uint8_t {
                UNKNOWN  = 0, ///< none / unknown
                OPTICAL  = 1, ///< wrist optical (PPG)
                EXTERNAL = 2, ///< external BLE strap
                /// Electrocardiogram. Settled by the kernel HR arbiter assigning a
                /// value; if it assigns 3 to something else, every frame already
                /// written is mislabelled.
                ECG      = 3,
            };

            static_assert(static_cast<uint8_t>(Source::UNKNOWN) ==
                          static_cast<uint8_t>(HeartRateEx::Source::UNKNOWN),
                          "RR_INTERVAL Source::UNKNOWN must match HEART_RATE_EX");
            static_assert(static_cast<uint8_t>(Source::OPTICAL) ==
                          static_cast<uint8_t>(HeartRateEx::Source::OPTICAL),
                          "RR_INTERVAL Source::OPTICAL must match HEART_RATE_EX");
            static_assert(static_cast<uint8_t>(Source::EXTERNAL) ==
                          static_cast<uint8_t>(HeartRateEx::Source::EXTERNAL),
                          "RR_INTERVAL Source::EXTERNAL must match HEART_RATE_EX");

            /**
             * @brief Continuity / quality flag bits (FLAGS field).
             * @note  Frozen, including DETECTOR_STAMPED's polarity: clearing a bit
             *        must be the claim-nothing state, since that is what a producer
             *        populating nothing leaves behind.
             */
            enum Flag : uint32_t {
                DISCONTINUITY    = 1u << 0, ///< not contiguous with the previous interval (gap / reconnect / first)
                ARTIFACT_SUSPECT = 1u << 1, ///< producer flags this interval as likely artefactual
                NO_SKIN_CONTACT  = 1u << 2, ///< strap reports loss of skin contact (0x2A37 contact bit)
                DETECTOR_STAMPED = 1u << 3, ///< timestamp is a detector-observed beat instant, not reconstructed from arrival
            };

            /**
             * @brief Result of checkContinuity().
             * @note  Frozen values; a consumer may log or persist them.
             * @note  Four outcomes rather than a bool because they want different
             *        responses: a GAP invalidates the window it falls in, a
             *        REORDERED says the stream cannot be accumulated in arrival
             *        order at all, and UNUSABLE says nothing was measured and no
             *        window need be discarded.
             */
            enum class Continuity : uint8_t {
                UNUSABLE   = 0, ///< nothing can be said: malformed frame, missing stamp, or a value outside the comparable range
                CONTIGUOUS = 1, ///< the observed gap agrees with rr_ms inside the budget
                GAP        = 2, ///< the gap exceeds rr_ms by more than the budget, or the producer declared DISCONTINUITY
                REORDERED  = 3, ///< the gap falls short by more than the budget
            };

            /**
             * @name Continuity budget terms
             * @brief The budget's inputs. Published so tests can pin them, NOT so a
             *        consumer can apply one by hand: the check is checkContinuity().
             * @{
             */

            /// Two quanta of getTimestampUs()'s 1 us resolution, two because a
            /// difference of two stamps carries the error of both. Never binds.
            static constexpr float kTimestampQuantisationMs = 0.002f;

            /// The fastest rate this contract UNDERTAKES to detect a lost beat at —
            /// not the fastest rate at which it happens to, which is higher and moves
            /// with kLostBeatMarginFraction, so do not build on it. Bounds detection,
            /// not validity: a faster stream parses normally. 200 is 220-minus-age at
            /// age 20.
            static constexpr float kGuaranteedDetectableBpm = 200.0f;

            /// The shortest whole interval a lost beat can add at that rate. The
            /// budget must stay strictly under it or loss is undetectable.
            static constexpr float kShortestLostBeatMs = 60000.0f / kGuaranteedDetectableBpm;

            /// Keeps the budget clear of the ceiling, so a beat that ran slightly
            /// fast is not absorbed. No measurement behind the magnitude: it would
            /// take beat-to-beat variation measured at kGuaranteedDetectableBpm, and
            /// it pulls both ways — a larger margin narrows the budget and buys false
            /// positives, while raising the rate above which a lost beat is silent.
            static constexpr float kLostBeatMarginFraction = 0.10f;

            /// The budget every frame gets, whatever it claims about its stamping.
            /// A window, not a bound: see continuityToleranceMs() for how a consumer
            /// that can do better supplies its own.
            static constexpr float kDefaultContinuityToleranceMs =
                    kShortestLostBeatMs - (kShortestLostBeatMs * kLostBeatMarginFraction);

            /// Upper bound on an interval, and on a caller-supplied budget, that
            /// checkContinuity() will compare at all: the span of Data's uint32
            /// millisecond timestamp field. Chosen from what the timestamps can mean,
            /// which keeps it well clear of where the float-to-int64 conversion below
            /// would become undefined.
            static constexpr float kMaxComparableRrMs = 4294967296.0f; // 2^32 ms

            /// The largest value the accessor's arithmetic can produce from Data's
            /// two uint32 fields, which is what keeps the signed conversion below in
            /// range. Deliberately looser than any conforming stamp: tightening it
            /// would only reject a prevUs no conforming producer can emit.
            static constexpr uint64_t kMaxTimestampUs =
                    (0xFFFFFFFFull * 1000ull) + 0xFFFFFFFFull;

            // The budget has to sit between floor and ceiling or it separates
            // nothing: at or above the ceiling a lost beat is undetectable, at or
            // below the floor it could not absorb the stamps themselves.
            static_assert(kDefaultContinuityToleranceMs < kShortestLostBeatMs,
                          "a budget at or above the shortest lost beat cannot detect loss");
            static_assert(kDefaultContinuityToleranceMs > kTimestampQuantisationMs,
                          "a budget below the timestamp quantum could not absorb the stamps themselves");
            /** @} */

            /**
             * @brief Construct a new RrInterval parser over one frame.
             * @param data Sensor data view holding at least the RR_MS float.
             */
            RrInterval(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Whether the frame carries a usable R-R interval.
             * @note  Shape AND value: a NaN / infinite / zero / negative interval
             *        is not a slow heartbeat but a malformed frame, and would put
             *        NaN into every downstream HRV accumulator. Every accessor
             *        that reads the frame gates on this, timestamps included, so
             *        such a frame reads as absent — metadata describes an interval,
             *        and a confident Source beside a NaN would invite a consumer to
             *        act on it.
             *        Lenient on the upper field bound, so a producer may append.
             */
            bool isDataValid() const
            {
                return (mData.getFieldCount() >= 1) &&
                       std::isfinite(mData.f[Field::RR_MS]) &&
                       (mData.f[Field::RR_MS] > 0.0f);
            }

            /**
             * @brief The R-R interval in milliseconds (0 if invalid).
             */
            float getRrMs() const
            {
                return isDataValid() ? mData.f[Field::RR_MS] : 0.0f;
            }

            /**
             * @brief Implied instantaneous heart rate (bpm) for convenience.
             * @note  Single-beat rate, NOT a smoothed HR and not a "max HR". 0 if
             *        the interval is invalid.
             * @note  No physiological range check: an implausible but finite
             *        interval (5 ms, or 40 s) passes through, because artefact
             *        policy is the consumer's — see ARTIFACT_SUSPECT. Only
             *        representability is guarded, a tiny enough interval making
             *        60000/rr overflow to infinity, which is not a reading; that
             *        reads 0 while getRrMs() still returns the raw interval.
             */
            float getBpm() const
            {
                const float rr = getRrMs();
                if (rr <= 0.0f) {
                    return 0.0f;
                }
                const float bpm = 60000.0f / rr;
                return std::isfinite(bpm) ? bpm : 0.0f;
            }

            /**
             * @brief Producer of this interval, or UNKNOWN if not supplied (or if
             *        the frame is malformed — see isDataValid()).
             */
            Source getSource() const
            {
                if (!isDataValid() || (mData.getFieldCount() <= Field::SOURCE)) {
                    return Source::UNKNOWN;
                }
                // The union's integer member: no narrowing, so any value a producer
                // can write is representable and unknown values fall to UNKNOWN.
                switch (mData.u[Field::SOURCE]) {
                    case static_cast<uint32_t>(Source::OPTICAL):  return Source::OPTICAL;
                    case static_cast<uint32_t>(Source::EXTERNAL): return Source::EXTERNAL;
                    case static_cast<uint32_t>(Source::ECG):      return Source::ECG;
                    default:                                      return Source::UNKNOWN;
                }
            }

            /**
             * @brief True if this interval is NOT contiguous with the previous one
             *        (gap, reconnect, or first interval). Absent flags read as
             *        contiguous.
             */
            bool hasDiscontinuity() const
            {
                return (flags() & Flag::DISCONTINUITY) != 0u;
            }

            /**
             * @brief True if the producer marks this interval as artefact-suspect.
             */
            bool isArtifactSuspect() const
            {
                return (flags() & Flag::ARTIFACT_SUSPECT) != 0u;
            }

            /**
             * @brief True if the strap reports loss of skin contact for this
             *        reading. False when contact is good or unreported.
             */
            bool isSkinContactLost() const
            {
                return (flags() & Flag::NO_SKIN_CONTACT) != 0u;
            }

            /**
             * @brief True if the producer asserts this frame's timestamp is a
             *        detector-observed beat instant rather than one reconstructed
             *        from a notification arrival.
             * @note  Informational today — the budget is the same either way — so
             *        setting it cannot provoke a false gap. Report it truthfully
             *        regardless: it is the only record of how the stamp was obtained,
             *        and a consumer may key its own budget on it via the two-argument
             *        checkContinuity().
             */
            bool isDetectorStamped() const
            {
                return (flags() & Flag::DETECTOR_STAMPED) != 0u;
            }

            /**
             * @brief The continuity budget this frame calls for, in milliseconds.
             * @note  Exposed so a consumer overriding it can see what it overrides,
             *        and so the choice is inspectable in a log. Not a threshold to
             *        apply by hand — checkContinuity() is.
             * @note  Frame-dependent by signature, not by behaviour: every frame
             *        currently gets kDefaultContinuityToleranceMs.
             */
            float continuityToleranceMs() const
            {
                return kDefaultContinuityToleranceMs;
            }

            /**
             * @brief Whether this interval continues from the beat that ended the
             *        previous one, using the budget this frame calls for.
             * @param prevUs getTimestampUs() of the previous frame whose
             *               isDataValid() held — see CONSUMER OBLIGATION.
             * @note  See CONTINUITY at the top of this file. A frame declaring
             *        DISCONTINUITY reads
             *        GAP whatever the stamps say — including with no previous stamp
             *        to compare against, since the declaration is producer knowledge
             *        that does not rest on them. So the first frame of a stream
             *        reads GAP if it declares one and UNUSABLE if it does not, and
             *        both are right: there is no window to discard either way.
             *        hasDiscontinuity() still distinguishes a declared gap from a
             *        detected one.
             */
            Continuity checkContinuity(uint64_t prevUs) const
            {
                return checkContinuity(prevUs, continuityToleranceMs());
            }

            /**
             * @name A narrower stamp is refused rather than widened
             * @brief getTimestamp() and getTimestampUs() sit next to each other,
             *        differ by a factor of 1000, and both reach uint64_t without a
             *        diagnostic — so passing the millisecond one returns a
             *        confident GAP on a contiguous pair, in the single operation
             *        this contract exists to stop consumers writing themselves. A
             *        millisecond stamp is indistinguishable at runtime from a
             *        microsecond stamp of a session 1000x younger.
             *
             *        Deleted on WIDTH rather than on uint32_t: a stamp is always
             *        uint64_t here, and matching that one type would leave
             *        checkContinuity(0ull) ambiguous wherever uint64_t is not
             *        unsigned long long. The predicate is a whitelist — an unsigned
             *        integral at least as wide as uint64_t — because a blacklist
             *        admits the next category by omission. No type predicate reaches
             *        the unit mistake itself: checkContinuity(5000ull) still reads a
             *        confident GAP. Cost: the stamp needs an unsigned 64-bit spelling,
             *        so the zero a consumer starts from is checkContinuity(0ull), and
             *        a µs count from <chrono> needs one cast.
             * @{
             */
            template <typename T, typename = typename std::enable_if<
                                          !(std::is_integral<T>::value &&
                                            !std::is_signed<T>::value &&
                                            (sizeof(T) >= sizeof(uint64_t)))>::type>
            Continuity checkContinuity(T prevNotMicroseconds) const = delete;

            template <typename T, typename = typename std::enable_if<
                                          !(std::is_integral<T>::value &&
                                            !std::is_signed<T>::value &&
                                            (sizeof(T) >= sizeof(uint64_t)))>::type>
            Continuity checkContinuity(T prevNotMicroseconds, float toleranceMs) const = delete;

            /**
             * @brief The transposition, refused from the budget's side.
             * @note  A budget is float milliseconds. checkContinuity(budgetMs, prevUs)
             *        otherwise puts the stamp in the budget and the budget in the
             *        stamp.
             *
             *        PARTIAL, and the limit bounds what any overload set can do here:
             *        the two predicates are duals, so a type refused as a STAMP is
             *        accepted as a BUDGET and the transposition survives there — an
             *        int64_t or <chrono> stamp, a double, a conversion operator, an
             *        enum. A budget in the wrong UNIT is unreachable by any signature.
             *        Closing the class wants a tagged budget type, which changes every
             *        call site.
             *
             *        Cost: a budget spelled as an unsigned 64-bit type no longer
             *        compiles. (x, 270), (x, 270.0f), (x, 270u) and (x, someSignedLong)
             *        are unaffected.
             */
            template <typename B, typename = typename std::enable_if<
                                          std::is_integral<B>::value &&
                                          !std::is_signed<B>::value &&
                                          (sizeof(B) >= sizeof(uint64_t))>::type>
            Continuity checkContinuity(uint64_t prevUs, B budgetNotAStamp) const = delete;
            /** @} */

            /**
             * @brief As above, against a caller-supplied budget.
             * @param prevUs      getTimestampUs() of the previous frame whose
             *                    isDataValid() held — see CONSUMER OBLIGATION.
             * @param toleranceMs the budget to allow, in milliseconds.
             * @note  For a consumer that knows the link parameters or has measured
             *        the jitter it sees. A negative, NaN or unrepresentable budget
             *        reads UNUSABLE rather than silently becoming zero — unless the
             *        frame declares DISCONTINUITY, which is answered before the budget
             *        is read, so a bad budget goes unreported on those frames. The
             *        comparison is SIGNED and in microseconds, because subtracting two
             *        unsigned stamps directly makes a reordered pair wrap to an
             *        enormous forward gap.
             */
            Continuity checkContinuity(uint64_t prevUs, float toleranceMs) const
            {
                if (!isDataValid()) {
                    return Continuity::UNUSABLE;
                }

                // A declared gap outranks anything the stamps can show.
                if (hasDiscontinuity()) {
                    return Continuity::GAP;
                }

                const uint64_t thisUs = getTimestampUs();
                // The microsecond field is a remainder, not a counter (see
                // TIMESTAMP); as a counter, no comparison it enters means anything.
                if ((thisUs - (static_cast<uint64_t>(getTimestamp()) * 1000ull)) >= 1000ull) {
                    return Continuity::UNUSABLE;
                }
                // Zero is "no stamp" throughout this class. Values above
                // kMaxTimestampUs did not come from getTimestampUs(), and admitting
                // them would put the signed conversion below out of range.
                if ((thisUs == 0ull) || (prevUs == 0ull) || (prevUs > kMaxTimestampUs)) {
                    return Continuity::UNUSABLE;
                }

                // Rejects NaN and negatives through the negated comparison, and
                // infinities through the upper bound.
                if (!(toleranceMs >= 0.0f) || (toleranceMs >= kMaxComparableRrMs)) {
                    return Continuity::UNUSABLE;
                }

                // isDataValid() established finite and positive; this is the
                // representability guard for the conversion, as in getBpm().
                const float rrMs = getRrMs();
                if (rrMs >= kMaxComparableRrMs) {
                    return Continuity::UNUSABLE;
                }

                const int64_t deltaUs = (thisUs >= prevUs)
                        ? static_cast<int64_t>(thisUs - prevUs)
                        : -static_cast<int64_t>(prevUs - thisUs);
                const int64_t rrUs  = static_cast<int64_t>(rrMs * 1000.0f);
                const int64_t tolUs = static_cast<int64_t>(toleranceMs * 1000.0f);
                const int64_t errUs = deltaUs - rrUs;

                if (errUs > tolUs) {
                    return Continuity::GAP;
                }
                if (errUs < -tolUs) {
                    return Continuity::REORDERED;
                }
                return Continuity::CONTIGUOUS;
            }

            uint32_t getTimestamp() const
            {
                return isDataValid() ? mData.getTimestamp() : 0;
            }

            uint64_t getTimestampUs() const
            {
                return isDataValid() ? mData.getTimestampUs() : 0;
            }

            /**
             * @brief Number of expected fields (3).
             * @note  The DELIVERY STRIDE a producer registers, not the parse
             *        minimum (see isDataValid()). Unpopulated metadata fields are
             *        zero, which reads as "not reported".
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            /**
             * @brief Raw FLAGS word, or 0 when absent or the frame is malformed.
             * @note  Every flag predicate reads through this, so a malformed frame
             *        reports no flags rather than the last producer's bits.
             */
            uint32_t flags() const
            {
                if (!isDataValid() || (mData.getFieldCount() <= Field::FLAGS)) {
                    return 0u;
                }
                return mData.u[Field::FLAGS];
            }

            const SDK::Sensor::DataView mData;
        }; /* class RrInterval */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* SDK_SENSORLAYER_DATAPARSERS_SENSOR_DATA_PARSER_RR_INTERVAL_HPP */
