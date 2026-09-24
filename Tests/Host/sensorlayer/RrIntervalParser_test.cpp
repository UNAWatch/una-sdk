/**
 * Host unit tests for SensorDataParser::RrInterval — the opt-in beat-to-beat
 * R-R interval frame (one interval per frame; optional appended source + flags
 * fields, lenient field-count validation like HeartRateEx).
 */

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "SDK/SensorLayer/SensorData.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/SensorLayer/SensorDataView.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserRrInterval.hpp"

using RrInterval = SDK::SensorDataParser::RrInterval;

namespace {

// A SDK::Sensor::Data with room for rr_ms + source + flags, and one spare for
// the "extra trailing field" forward-compat case.
struct RrData {
    alignas(SDK::Sensor::Data) uint8_t buf[sizeof(SDK::Sensor::Data) +
            3 * sizeof(SDK::Sensor::Data::Field)] {};
    SDK::Sensor::Data* operator->() { return data(); }
    SDK::Sensor::Data* data() { return reinterpret_cast<SDK::Sensor::Data*>(buf); }
};

int src(RrInterval::Source s) { return static_cast<int>(s); }

int cont(RrInterval::Continuity c) { return static_cast<int>(c); }

// One RR frame stamped at a given microsecond instant, so the continuity tests
// go through real frames and the real accessors rather than through arithmetic
// on literals.
struct Beat {
    RrData   d;
    uint16_t fields;

    Beat(float rrMs, uint64_t tsUs, uint32_t flags,
         uint16_t nFields = RrInterval::COUNT)
        : fields(nFields)
    {
        d->mTimeStamp                    = static_cast<uint32_t>(tsUs / 1000ull);
        d->mTimeStampUs                  = static_cast<uint32_t>(tsUs % 1000ull);
        d->mValue[RrInterval::RR_MS].f   = rrMs;
        d->mValue[RrInterval::FLAGS].u32 = flags;
    }

    RrInterval parser() { return RrInterval(SDK::Sensor::DataView(*d.data(), fields)); }
};

uint64_t msToUs(float ms) { return static_cast<uint64_t>(ms * 1000.f); }

// Whether checkContinuity() can be called with a stamp of type T at all. A
// deleted overload is chosen by overload resolution and then makes the call
// ill-formed, which in this immediate context is a substitution failure rather
// than a hard error — so this detects the deletion instead of failing to compile.
template <typename T, typename = void>
struct AcceptsStamp : std::false_type {};

template <typename T>
struct AcceptsStamp<T, std::void_t<decltype(std::declval<const RrInterval&>()
                                                    .checkContinuity(std::declval<T>()))>>
    : std::true_type {};

// Whether a BUDGET of type B is accepted alongside a valid stamp.
template <typename B, typename = void>
struct AcceptsBudget : std::false_type {};

template <typename B>
struct AcceptsBudget<B, std::void_t<decltype(std::declval<const RrInterval&>()
                                                     .checkContinuity(uint64_t(1),
                                                                      std::declval<B>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct AcceptsStampAndBudget : std::false_type {};

template <typename T>
struct AcceptsStampAndBudget<T, std::void_t<decltype(std::declval<const RrInterval&>()
                                                             .checkContinuity(std::declval<T>(),
                                                                              1.f))>>
    : std::true_type {};

enum UnscopedStamp : unsigned { kSessionStartMs = 5000 };

struct ConvertsToStamp { operator uint64_t() const { return 5000ull; } };

} // namespace

TEST(RrIntervalParser, WireNumbersAreFrozen)
{
    // The frozen numbers. They ARE the contract -- a producer and a consumer
    // compiled from different revisions have only these and the field types to
    // agree on -- and the rest of the suite writes each fixture through the same
    // symbols it reads back, so renumbering Field, Source or Flag would be
    // invisible to it.
    //
    // Source::ECG is convention rather than settled, the arbiter having no value
    // for it. Pinned here anyway: until it does, this file is the only thing that
    // says what 3 means.
    EXPECT_EQ(static_cast<uint32_t>(SDK::Sensor::Type::RR_INTERVAL), 0x44u);

    EXPECT_EQ(RrInterval::RR_MS,  0);
    EXPECT_EQ(RrInterval::SOURCE, 1);
    EXPECT_EQ(RrInterval::FLAGS,  2);
    EXPECT_EQ(RrInterval::COUNT,  3);

    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::UNKNOWN),  0u);
    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::OPTICAL),  1u);
    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::EXTERNAL), 2u);
    EXPECT_EQ(static_cast<uint32_t>(RrInterval::Source::ECG),      3u);

    EXPECT_EQ(RrInterval::Flag::DISCONTINUITY,    1u << 0);
    EXPECT_EQ(RrInterval::Flag::ARTIFACT_SUSPECT, 1u << 1);
    EXPECT_EQ(RrInterval::Flag::NO_SKIN_CONTACT,  1u << 2);
    EXPECT_EQ(RrInterval::Flag::DETECTOR_STAMPED, 1u << 3);

    // Polarity is part of the wire contract, not an implementation detail: the
    // bit has to mean "detector-stamped", so that clearing it — which is what a
    // producer that populates nothing does — is the claim-nothing state. No
    // budget currently turns on it, but the moment one does, a bit meaning
    // "arrival-reconstructed" would hand the tightest budget to the least
    // informative producer, and by then the numbering is frozen.
    EXPECT_EQ(cont(RrInterval::Continuity::UNUSABLE),   0);
    EXPECT_EQ(cont(RrInterval::Continuity::CONTIGUOUS), 1);
    EXPECT_EQ(cont(RrInterval::Continuity::GAP),        2);
    EXPECT_EQ(cont(RrInterval::Continuity::REORDERED),  3);
}

TEST(RrIntervalParser, ParsesAFrameWrittenWithRawWireIndices)
{
    // Written the way a producer built from the documentation writes it: literal
    // offsets, literal values, no symbol shared with the reader, so a
    // reader/writer disagreement about the layout has somewhere to surface.
    RrData m;
    m->mValue[0].f   = 855.f;
    m->mValue[1].u32 = 2u; // EXTERNAL
    m->mValue[2].u32 = 5u; // DISCONTINUITY | NO_SKIN_CONTACT

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_FLOAT_EQ(p.getRrMs(), 855.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::EXTERNAL));
    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
    EXPECT_TRUE(p.isSkinContactLost());
}

TEST(RrIntervalParser, FieldsNumberIsTheFullStride)
{
    // Not the parse minimum: a producer registers this as its Driver stride, and
    // registering fewer fields would make SOURCE/FLAGS permanently undeliverable.
    EXPECT_EQ(RrInterval::getFieldsNumber(), 3);
    EXPECT_EQ(RrInterval::getFieldsNumber(), RrInterval::COUNT);
}

TEST(RrIntervalParser, OneFieldMinimalFrame)
{
    RrData m;
    m->mTimeStamp = 1000;
    m->mValue[RrInterval::RR_MS].f = 855.f;
    // Non-zero, and past the delivered end: a Driver republishes one DataSample, so
    // the word beyond the stride is the PREVIOUS beat's source, not a fresh zero.
    // Without this the frame's undelivered SOURCE reads zero either way and the
    // field-count guard in getSource() is pinned by nothing.
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);

    // A short frame still parses — the upper field bound is lenient.
    RrInterval p(SDK::Sensor::DataView(*m.data(), 1));

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 855.f);
    EXPECT_FLOAT_EQ(p.getBpm(), 60000.f / 855.f);
    EXPECT_EQ(p.getTimestamp(), 1000u);
    // Metadata absent -> safe defaults, never a false gap.
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
    EXPECT_FALSE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
}

TEST(RrIntervalParser, MicrosecondTailSurvivesOnAValidFrame)
{
    // The contract tells a producer to stamp in the microsecond domain, so the
    // sub-millisecond tail has to reach a consumer intact — an interval is
    // fractional in milliseconds and the whole point of the mandate is that the
    // fraction is not thrown away.
    RrData m;
    m->mTimeStamp                  = 1000;
    m->mTimeStampUs                = 469; // 1000.469 ms
    m->mValue[RrInterval::RR_MS].f = 855.f;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 1));

    EXPECT_EQ(p.getTimestamp(), 1000u);
    EXPECT_EQ(p.getTimestampUs(), 1000469ull);
}

TEST(RrIntervalParser, ZeroMetadataReadsAsNotReported)
{
    // The invariant that makes the producer obligation survivable: a full-stride
    // frame whose metadata words are zero degrades to "no information" rather
    // than to a confident wrong answer. Zero is the value a producer that
    // registers the stride and populates nothing leaves behind, whoever wrote it.
    RrData m;
    m->mValue[RrInterval::RR_MS].f = 855.f;

    RrInterval p(SDK::Sensor::DataView(*m.data(), RrInterval::COUNT));

    EXPECT_TRUE(p.isDataValid());
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
    EXPECT_FALSE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
    EXPECT_FALSE(p.isSkinContactLost());
    // Including the continuity claim: unclaimed, so the loose budget.
    EXPECT_FALSE(p.isDetectorStamped());
    EXPECT_FLOAT_EQ(p.continuityToleranceMs(),
                    RrInterval::kDefaultContinuityToleranceMs);
}

TEST(RrIntervalParser, ThreeFieldExternalWithGap)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 412.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::DISCONTINUITY;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_FLOAT_EQ(p.getRrMs(), 412.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::EXTERNAL));
    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
}

TEST(RrIntervalParser, ArtifactFlagIndependentOfGap)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 800.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::ECG);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::ARTIFACT_SUSPECT;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::ECG));
    EXPECT_TRUE(p.isArtifactSuspect());
    EXPECT_FALSE(p.hasDiscontinuity());
}

TEST(RrIntervalParser, SkinContactLostFlag)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 900.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::NO_SKIN_CONTACT;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_TRUE(p.isSkinContactLost());
    EXPECT_FALSE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
}

TEST(RrIntervalParser, UnknownSourceForOutOfRangeValue)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 700.f;
    m->mValue[RrInterval::SOURCE].u32 = 9u; // not a known Source

    RrInterval p(SDK::Sensor::DataView(*m.data(), 2));

    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
}

TEST(RrIntervalParser, KnownFlagsCombine)
{
    // The realistic case for a strap reconnect: the first beat after the gap is
    // both discontinuous and worth distrusting. Each predicate must read its own
    // bit, not the mask being non-zero.
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 850.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::DISCONTINUITY |
                                        RrInterval::Flag::ARTIFACT_SUSPECT;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_TRUE(p.isArtifactSuspect());
    EXPECT_FALSE(p.isSkinContactLost());
}

TEST(RrIntervalParser, AllFlagsSetAtOnce)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f   = 850.f;
    m->mValue[RrInterval::FLAGS].u32 = RrInterval::Flag::DISCONTINUITY |
                                       RrInterval::Flag::ARTIFACT_SUSPECT |
                                       RrInterval::Flag::NO_SKIN_CONTACT |
                                       RrInterval::Flag::DETECTOR_STAMPED;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_TRUE(p.isArtifactSuspect());
    EXPECT_TRUE(p.isSkinContactLost());
    EXPECT_TRUE(p.isDetectorStamped());
}

TEST(RrIntervalParser, UndefinedFlagBitsDoNotDisturbKnownOnes)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f   = 800.f;
    // A sparse high bit alongside bit0 — the case a float-carried mask rounds
    // away, and the reason FLAGS is u32.
    m->mValue[RrInterval::FLAGS].u32 = RrInterval::Flag::DISCONTINUITY | (1u << 24);

    RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

    EXPECT_TRUE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isArtifactSuspect());
    EXPECT_FALSE(p.isSkinContactLost());
}

TEST(RrIntervalParser, UnusableRrValuesMakeTheFrameInvalid)
{
    // Not slow heartbeats — malformed frames. Letting these through would put
    // NaN/inf into every downstream HRV accumulator.
    const float bad[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        0.f,
        -800.f,
    };

    for (float v : bad) {
        RrData m;
        m->mTimeStamp                     = 1000;
        m->mValue[RrInterval::RR_MS].f    = v;
        m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
        m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::DISCONTINUITY |
                                            RrInterval::Flag::NO_SKIN_CONTACT;

        RrInterval p(SDK::Sensor::DataView(*m.data(), 3));

        EXPECT_FALSE(p.isDataValid());
        EXPECT_FLOAT_EQ(p.getRrMs(), 0.f);
        EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
        // The whole frame reads as absent — timestamps and metadata included.
        // Populated metadata beside a malformed interval describes nothing, and
        // handing back a confident EXTERNAL would invite a consumer to trust the
        // rest of the frame.
        EXPECT_EQ(p.getTimestamp(), 0u);
        EXPECT_EQ(p.getTimestampUs(), 0u);
        EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
        EXPECT_FALSE(p.hasDiscontinuity());
        EXPECT_FALSE(p.isArtifactSuspect());
        EXPECT_FALSE(p.isSkinContactLost());
        EXPECT_FALSE(p.isDetectorStamped());
    }
}

TEST(RrIntervalParser, ATwoFieldFrameHasNoFlagsWordToRead)
{
    // The other side of the lenient field bound, and the only stride between the
    // one-field minimum and the full three that any accessor has to reason about.
    // FLAGS is past the end here, so every flag predicate must read as absent
    // rather than reaching for a word the producer did not deliver — which, since
    // a Driver republishes one DataSample, is the PREVIOUS beat's flags. A stale
    // DISCONTINUITY read out of bounds is exactly what the per-frame write rule
    // exists to prevent, so the bound has to be pinned at the stride that tests it.
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 800.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::EXTERNAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::DISCONTINUITY |
                                        RrInterval::Flag::DETECTOR_STAMPED;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 2));

    EXPECT_TRUE(p.isDataValid());
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::EXTERNAL)); // delivered
    EXPECT_FALSE(p.hasDiscontinuity());                              // not delivered
    EXPECT_FALSE(p.isArtifactSuspect());
    EXPECT_FALSE(p.isSkinContactLost());
    EXPECT_FALSE(p.isDetectorStamped());
    // And the budget follows the absent claim, not the undelivered word.
    EXPECT_FLOAT_EQ(p.continuityToleranceMs(),
                    RrInterval::kDefaultContinuityToleranceMs);
}

TEST(RrIntervalParser, AppendedFieldLeavesTheKnownOnesAlone)
{
    // The forward-compatibility claim the contract rests on: a later producer may
    // append a field (a graded confidence, say) and register a wider stride, and
    // an app built against today's layout must be unaffected. This is what makes
    // deferring such a field safe rather than a decision that has to be made now.
    RrData m;
    m->mValue[RrInterval::RR_MS].f    = 855.f;
    m->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(RrInterval::Source::OPTICAL);
    m->mValue[RrInterval::FLAGS].u32  = RrInterval::Flag::ARTIFACT_SUSPECT;
    m->mValue[RrInterval::COUNT].u32  = 0xDEADBEEFu; // a field this build knows nothing about

    RrInterval p(SDK::Sensor::DataView(*m.data(), RrInterval::COUNT + 1));

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 855.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::OPTICAL));
    EXPECT_TRUE(p.isArtifactSuspect());
    EXPECT_FALSE(p.hasDiscontinuity());
    EXPECT_FALSE(p.isSkinContactLost());
}

// ---------------------------------------------------------------------------
// Continuity. These pin the relationships between the budget's named terms, not
// the numbers they produce: every budget below is recomputed from its terms, so
// changing a term either moves both sides together or breaks a test.
// ---------------------------------------------------------------------------

TEST(RrIntervalContinuity, TheBudgetsFallOutOfTheNamedTerms)
{
    // The ceiling. A lost beat adds one whole interval, so a budget detects loss
    // only while it stays under the shortest interval that could go missing —
    // recomputed from the policy rate here rather than restated, so moving
    // kGuaranteedDetectableBpm moves the ceiling with it.
    EXPECT_FLOAT_EQ(RrInterval::kShortestLostBeatMs,
                    60000.f / RrInterval::kGuaranteedDetectableBpm);

    // The floor. getTimestampUs() resolves to 1 us and a difference of two
    // stamps carries at most two quanta.
    EXPECT_FLOAT_EQ(RrInterval::kTimestampQuantisationMs, 2.f * 0.001f);

    // The budget sits strictly between floor and ceiling. Without that the check
    // cannot separate a jittered pair from a lost beat at all, and this is the
    // whole claim the derivation makes.
    EXPECT_GT(RrInterval::kDefaultContinuityToleranceMs,
              RrInterval::kTimestampQuantisationMs);
    EXPECT_LT(RrInterval::kDefaultContinuityToleranceMs,
              RrInterval::kShortestLostBeatMs);

    // The budget is NOT a sum of terms, and that is the finding rather than an
    // omission. Neither stamping class brackets: for an arrival-reconstructed
    // frame the dominant term is the transport jitter between two arrivals, which
    // nothing in this repository exposes, configures or bounds; for a
    // detector-stamped frame it is the detector-to-stamp jitter, which nothing
    // bounds either, because there is no detector. So the budget is the ceiling
    // less its margin — the whole of the window available — rather than a figure
    // claimed to cover anything.
    EXPECT_FLOAT_EQ(RrInterval::kDefaultContinuityToleranceMs,
                    RrInterval::kShortestLostBeatMs *
                            (1.f - RrInterval::kLostBeatMarginFraction));
}

TEST(RrIntervalContinuity, ThereIsExactlyOneBudgetAndNoFrameChangesIt)
{
    // NOTHING in a frame moves the budget. That is what makes a producer
    // populating nothing safe, and a reintroduced per-stamping-class budget would
    // have to break this deliberately rather than by accident.
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;
    // An offset a tight per-class budget would have separated on, and this one
    // absorbs.
    const uint64_t offUs  = msToUs(50.f);
    ASSERT_LT(offUs, msToUs(RrInterval::kDefaultContinuityToleranceMs));
    const uint64_t thisUs = prevUs + 800000ull + offUs;

    const uint32_t flagSets[] = {
        0u,
        RrInterval::Flag::DETECTOR_STAMPED,
        RrInterval::Flag::ARTIFACT_SUSPECT | RrInterval::Flag::NO_SKIN_CONTACT,
        RrInterval::Flag::DETECTOR_STAMPED | RrInterval::Flag::ARTIFACT_SUSPECT |
                RrInterval::Flag::NO_SKIN_CONTACT,
    };

    for (uint32_t flags : flagSets) {
        for (uint16_t stride : { uint16_t(1), uint16_t(2),
                                 uint16_t(RrInterval::COUNT) }) {
            Beat b(rr, thisUs, flags, stride);
            EXPECT_FLOAT_EQ(b.parser().continuityToleranceMs(),
                            RrInterval::kDefaultContinuityToleranceMs)
                    << "flags=" << flags << " stride=" << stride;
            EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                      cont(RrInterval::Continuity::CONTIGUOUS))
                    << "flags=" << flags << " stride=" << stride;
        }
    }

    // The flag is still readable — it is the only record of how the stamp was
    // obtained, and a consumer may key its OWN budget on it through the
    // two-argument overload even though this class does not.
    Beat stamped(rr, thisUs, RrInterval::Flag::DETECTOR_STAMPED);
    EXPECT_TRUE(stamped.parser().isDetectorStamped());
    EXPECT_EQ(cont(stamped.parser().checkContinuity(prevUs, 20.f)),
              cont(RrInterval::Continuity::GAP));
}

TEST(RrIntervalContinuity, TheStampBoundIsLooseAndTheMicrosecondFieldIsARemainder)
{
    // kMaxTimestampUs is the gate on prevUs, and it is the ceiling of the accessor's
    // ARITHMETIC over Data's two uint32 fields -- what keeps the signed conversion in
    // range -- not the reachable maximum, which is lower because the microsecond
    // field is a remainder. Holding the loose value refuses no prevUs a
    // conforming producer can send; the remainder is what the verdicts rest on.
    RrData m;
    m->mTimeStamp                  = 0xFFFFFFFFu;
    m->mTimeStampUs                = 0xFFFFFFFFu;
    m->mValue[RrInterval::RR_MS].f = 800.f;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 1));

    EXPECT_EQ(p.getTimestampUs(), RrInterval::kMaxTimestampUs);
    // And such a stamp is accepted as a previous instant rather than rejected as
    // impossible: a bound narrower than the accessor's range would read this as
    // "not from this API".
    EXPECT_LE(p.getTimestampUs(), RrInterval::kMaxTimestampUs);

    Beat b(800.f, 5000000ull, 0u);
    EXPECT_EQ(cont(b.parser().checkContinuity(RrInterval::kMaxTimestampUs)),
              cont(RrInterval::Continuity::REORDERED));
    EXPECT_EQ(cont(b.parser().checkContinuity(RrInterval::kMaxTimestampUs + 1ull)),
              cont(RrInterval::Continuity::UNUSABLE));

    // The remainder is a PRECONDITION of the comparison, not a convention a writer
    // happens to follow: a full microsecond count doubles the apparent interval and
    // can order two frames backwards, so such a frame gets no verdict rather than a
    // confident wrong one. Without the guard the pair below reads GAP, silently. It
    // is a backstop only -- it cannot inspect prevUs, and a full counter that lands
    // under 1000 by chance still gets a verdict.
    {
        RrData full;
        full->mTimeStamp                  = 5800u;
        full->mTimeStampUs                = 5800000u; // a full counter, not a tail
        full->mValue[RrInterval::RR_MS].f = 800.f;
        RrInterval full_p(SDK::Sensor::DataView(*full.data(), 1));

        RrData prev;
        prev->mTimeStamp                  = 5000u;
        prev->mTimeStampUs                = 5000000u;
        prev->mValue[RrInterval::RR_MS].f = 800.f;
        RrInterval q(SDK::Sensor::DataView(*prev.data(), 1));

        EXPECT_EQ(cont(full_p.checkContinuity(q.getTimestampUs())),
                  cont(RrInterval::Continuity::UNUSABLE));
        // Both sides of the threshold, since every other case here sits orders of
        // magnitude clear of it.
        RrData edge;
        edge->mTimeStamp                  = 5800u;
        edge->mTimeStampUs                = 999u;
        edge->mValue[RrInterval::RR_MS].f = 800.f;
        RrInterval e(SDK::Sensor::DataView(*edge.data(), 1));
        EXPECT_NE(cont(e.checkContinuity(5000999ull)),
                  cont(RrInterval::Continuity::UNUSABLE)) << "999 us is a remainder";

        // And past where a 32-bit ms * 1000 product would wrap: the guard's
        // subtraction has to be done in 64 bits or a CONFORMING frame at a high
        // uptime reads UNUSABLE. 4294968 ms * 1000 overflows uint32.
        RrData high;
        high->mTimeStamp                  = 4294968u;
        high->mTimeStampUs                = 500u;
        high->mValue[RrInterval::RR_MS].f = 800.f;
        RrInterval h(SDK::Sensor::DataView(*high.data(), 1));
        EXPECT_NE(cont(h.checkContinuity(4294967700ull)),
                  cont(RrInterval::Continuity::UNUSABLE))
                << "a conforming frame past the 32-bit product's wrap";

        RrData over;
        over->mTimeStamp                  = 5800u;
        over->mTimeStampUs                = 1000u; // one microsecond past a remainder
        over->mValue[RrInterval::RR_MS].f = 800.f;
        RrInterval o(SDK::Sensor::DataView(*over.data(), 1));
        EXPECT_EQ(cont(o.checkContinuity(5001000ull)),
                  cont(RrInterval::Continuity::UNUSABLE)) << "1000 us is not";
    }
}

TEST(RrIntervalContinuity, TheComparableBoundIsTheSpanOfTheMillisecondField)
{
    // The other field-width bound, held for the same reason and by the same route:
    // kMaxComparableRrMs is the span of Data's uint32 millisecond field, and an
    // rr_ms at or above it did not come from a stream this API can measure.
    //
    // It needs its own line because nothing else reaches it. Every case that
    // exercises the bound — an interval too large to compare, a budget too large
    // to compare — spells it symbolically or picks a value orders of magnitude
    // clear of it, so halving the constant leaves all of them green while
    // silently refusing intervals and budgets a producer is free to send.
    EXPECT_FLOAT_EQ(RrInterval::kMaxComparableRrMs,
                    static_cast<float>(
                            static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1ull));
}

TEST(RrIntervalContinuity, ThePolicyTermsAreHeldToTheirStatedJustification)
{
    // The CHOSEN terms. The test above pins only the derivation's internal
    // consistency, and scaling a picked term scales both sides of it without
    // complaint — so each is held here to the reason given for it, and moving one
    // means editing that reason in the same change. The terms that follow from the
    // others are recomputed above; the two held to Data's field widths get a test
    // each.

    // kGuaranteedDetectableBpm is justified as covering a threshold protocol run to
    // exhaustion — the 220-minus-age maximum at age 20. That is an exact figure,
    // so it is pinned exactly rather than bracketed: a range would let the term
    // drift anywhere inside it, and every budget below is scaled from this one,
    // so 200 -> 240 would silently cut the published loose budget from 270 ms to
    // 225 ms with nothing to complain. Moving it means editing the reason in the
    // header and this line together.
    EXPECT_FLOAT_EQ(RrInterval::kGuaranteedDetectableBpm, 220.f - 20.f);
    // Lowering it is the tempting move, because it widens every budget and makes
    // false gaps go away; what it buys is giving up detection at rates people
    // reach. Raising it is the opposite trade, and the window closes entirely not
    // far above: a lost beat at 240 bpm is only 250 ms.
    EXPECT_LE(RrInterval::kGuaranteedDetectableBpm, 240.f);

    // kLostBeatMarginFraction is bounded rather than pinned, and the difference is
    // the point: the rate's rationale names a figure to pin it to, this one's
    // names only a reason the term must exist. So the bound is the honest test —
    // big enough to mean something, small enough to stay a margin rather than
    // become the budget. Inside that bracket this is the only assertion that holds
    // the term: the continuity cases are written in quantities derived from the
    // budget, so they move with it instead of complaining.
    EXPECT_GE(RrInterval::kLostBeatMarginFraction, 0.05f);
    EXPECT_LE(RrInterval::kLostBeatMarginFraction, 0.25f);
}

TEST(RrIntervalContinuity, EqualityIsUnusableWhichIsWhyThereIsABudgetAtAll)
{
    // An R-R value is a whole number of 1/1024 s ticks, so it is fractional in
    // milliseconds, and for this tick count not a whole number of microseconds
    // either. Equality is not something to rely on either way: a count divisible
    // by 16 does land on a whole microsecond, and a producer that floors its
    // arithmetic lands on the same truncation checkContinuity applies. The budget
    // is what the verdict rests on.
    const float    rr   = 876.f * 125.f / 128.f; // 876 ticks == 855.46875 ms
    const uint64_t rrUs = static_cast<uint64_t>(rr * 1000.f + 0.5f);

    EXPECT_NE(static_cast<float>(rrUs) / 1000.f, rr);

    // ...and the check reads it as contiguous regardless.
    const uint64_t prevUs = 10000000ull;
    Beat b(rr, prevUs + rrUs, 0u);
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
              cont(RrInterval::Continuity::CONTIGUOUS));
}

TEST(RrIntervalContinuity, JitterReadsContiguousAndALostBeatReadsAsAGap)
{
    // Through real frames and the exported operation — the three cases the
    // budget has to sort out, in the order they matter.
    const float    rr     = 876.f * 125.f / 128.f;
    const uint64_t prevUs = 10000000ull;
    const uint64_t rrUs   = static_cast<uint64_t>(rr * 1000.f + 0.5f);

    // Jitter just inside the budget, in both directions. A healthy stream must
    // never read as a lost frame: that false positive is the expensive error,
    // because it discards the HRV windows the strap was connected for.
    const uint64_t jitterUs = msToUs(RrInterval::kDefaultContinuityToleranceMs - 1.f);
    {
        Beat late(rr, prevUs + rrUs + jitterUs, 0u);
        EXPECT_EQ(cont(late.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
        Beat early(rr, prevUs + rrUs - jitterUs, 0u);
        EXPECT_EQ(cont(early.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
    }

    // A hole the size of the shortest interval the contract undertakes to detect
    // the loss of -- not a beat of the stream above, which is slower.
    {
        Beat lost(rr, prevUs + rrUs + msToUs(RrInterval::kShortestLostBeatMs), 0u);
        EXPECT_EQ(cont(lost.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::GAP));
    }
}

TEST(RrIntervalContinuity, TheBudgetIsInclusiveAtBothEdges)
{
    // Where the verdict actually changes. Every other case here probes a budget's
    // worth either side and so pins the middle of each band rather than its edge —
    // which leaves the comparison itself free to be off by one in either
    // direction. GAP is defined as exceeding rr_ms by MORE than the budget, so an
    // error of exactly the budget is still CONTIGUOUS, and the same at the
    // reordering edge. Both edges, and the first value past each.
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;
    const uint64_t rrUs   = 800000ull;
    const uint64_t tolUs  =
            static_cast<uint64_t>(RrInterval::kDefaultContinuityToleranceMs * 1000.f);

    {
        Beat at(rr, prevUs + rrUs + tolUs, 0u);
        EXPECT_EQ(cont(at.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS)) << "late by exactly the budget";
        Beat past(rr, prevUs + rrUs + tolUs + 1ull, 0u);
        EXPECT_EQ(cont(past.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::GAP)) << "one microsecond past it";
    }
    {
        Beat at(rr, prevUs + rrUs - tolUs, 0u);
        EXPECT_EQ(cont(at.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS)) << "early by exactly the budget";
        Beat past(rr, prevUs + rrUs - tolUs - 1ull, 0u);
        EXPECT_EQ(cont(past.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::REORDERED)) << "one microsecond past it";
    }
}

TEST(RrIntervalContinuity, AChainReadsContiguousWithinAndAcrossANotification)
{
    // The two arrangements a real arrival-reconstructing producer emits, walked
    // as a consumer walks them: previous stamp in hand, one verdict per frame.
    //
    // Within one notification the reconstructed instants are exactly rr apart by
    // construction, so those pairs carry no transport error at all. Across a
    // boundary the two instants hang off two different arrivals and the pair
    // inherits whatever sat between them — the case the loose budget exists for.
    // Both must read CONTIGUOUS, or a producer following the TIMESTAMP recipe
    // would see its own output reported as lossy.
    const float rr[] = { 812.f, 795.f, 838.f, 806.f };

    // One notification's worth, back-dated from a single arrival instant T.
    const uint64_t tArrive = 30000000ull;
    uint64_t       stamps[4];
    {
        uint64_t tail = 0ull; // SUM(rr[j] for j > i), accumulated backwards
        for (int i = 3; i >= 0; --i) {
            stamps[i] = tArrive - tail;
            tail += msToUs(rr[i]);
        }
    }

    uint64_t prevUs = stamps[0] - msToUs(rr[0]); // the beat before this burst
    for (int i = 0; i < 4; ++i) {
        Beat b(rr[i], stamps[i], 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS))
                << "within-notification pair " << i;
        prevUs = stamps[i];
    }

    // The next notification arrives a second later, and its first beat is late by
    // most of the loose budget — the boundary pair, which is the only place in an
    // arrival-reconstructed stream where transport jitter can show up.
    {
        const float    nextRr = 801.f;
        const uint64_t skew   = msToUs(RrInterval::kDefaultContinuityToleranceMs - 1.f);
        Beat           b(nextRr, prevUs + msToUs(nextRr) + skew, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
    }
}

TEST(RrIntervalContinuity, SeveralLostBeatsStillReadAsOneGap)
{
    // One lost beat is the threshold case; several is the common one, and the
    // verdict must not degrade as the hole grows. There is no "how many" in the
    // answer — a consumer discards the window either way — so the only thing that
    // must hold is that a bigger hole never reads as anything milder.
    const float    rr     = 800.f;
    const uint64_t prevUs = 20000000ull;

    for (int lost = 1; lost <= 5; ++lost) {
        const uint64_t thisUs = prevUs + msToUs(rr) +
                static_cast<uint64_t>(lost) * msToUs(rr);
        Beat b(rr, thisUs, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::GAP))
                << lost << " beats lost";
    }
}

TEST(RrIntervalContinuity, ALostBeatIsCaughtAtThePolicyRateAndHiddenAboveTheBudget)
{
    // The stated residual, and the one that costs a consumer something because it
    // is silent. It is NOT "above kGuaranteedDetectableBpm": loss is caught
    // wherever the interval that went missing still exceeds the budget, and the
    // margin puts that boundary above the rate. Describing the blind spot as
    // starting at the rate overstates it by the whole width of the margin, so all
    // three points are pinned here — caught at the rate, caught in the band above
    // it, silent past the boundary.
    const uint64_t prevUs = 5000000ull;

    // One whole beat missing, at a given rate. A lost beat adds the neighbouring
    // interval, so this is the shape the budget has to sort out.
    const auto lostBeatAt = [prevUs](float bpm) {
        const float    rr   = 60000.f / bpm;
        const uint64_t rrUs = static_cast<uint64_t>(rr * 1000.f);
        Beat b(rr, prevUs + rrUs + rrUs, 0u);
        return cont(b.parser().checkContinuity(prevUs));
    };

    // The undertaking itself, at the rate that names it.
    EXPECT_EQ(lostBeatAt(RrInterval::kGuaranteedDetectableBpm),
              cont(RrInterval::Continuity::GAP));

    // The band above it, where detection continues on the margin's account rather
    // than the contract's. This is what the header declines to state as a boundary,
    // because it moves with any term — but it must not be described as silent.
    const float boundaryBpm = 60000.f / RrInterval::kDefaultContinuityToleranceMs;
    ASSERT_GT(boundaryBpm, RrInterval::kGuaranteedDetectableBpm)
            << "the margin is what puts the boundary above the rate";
    EXPECT_EQ(lostBeatAt((RrInterval::kGuaranteedDetectableBpm + boundaryBpm) / 2.f),
              cont(RrInterval::Continuity::GAP));

    // And past the boundary it really is silent: the whole missing interval now
    // fits inside the budget, so a consumer is told nothing. Derived rather than
    // written as a rate, so no figure here needs maintaining alongside the header.
    EXPECT_EQ(lostBeatAt(boundaryBpm * 1.05f), cont(RrInterval::Continuity::CONTIGUOUS));
}

TEST(RrIntervalContinuity, TheMillisecondFieldWrapReadsAsOneReordering)
{
    // A stated residual, re-derived here rather than asserted: Data's mTimeStamp
    // is uint32 ms, so a session crossing ~49.7 days of uptime has exactly one
    // pair whose stamps straddle the wrap. Signed arithmetic sees the post-wrap
    // stamp as far EARLIER than its predecessor, so that pair reads REORDERED —
    // not GAP, which is what the unsigned subtraction would have produced.
    const float    rr      = 800.f;
    const uint64_t lastMs  = 0xFFFFFFFFull;            // the final representable ms
    const uint64_t prevUs  = lastMs * 1000ull + 600ull;
    const uint64_t afterUs = 400ull * 1000ull;         // wrapped: 400 ms past zero

    {
        Beat b(rr, afterUs, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::REORDERED));
    }

    // And it is ONE pair, not a lasting condition: the very next pair is drawn
    // from two post-wrap stamps and reads normally again.
    {
        const uint64_t nextUs = afterUs + msToUs(rr);
        Beat           b(rr, nextUs, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(afterUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
    }
}

TEST(RrIntervalContinuity, TheFirstFrameOfAStreamAndOfAReconnect)
{
    // Both "first" cases, which a consumer hits before it has anything to compare
    // against and again after every reconnect. Neither may read CONTIGUOUS, and
    // neither may leave a consumer accumulating across the boundary.
    const float rr = 800.f;

    // First frame of a stream, flags clear: no previous stamp exists, and 0 is what
    // a consumer has before it has seen one. Nothing can be said, so UNUSABLE.
    {
        Beat b(rr, 40000000ull, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(0ull)),
                  cont(RrInterval::Continuity::UNUSABLE));
    }

    // The same frame from a producer that declares the gap it knows about. The
    // declaration is producer knowledge that does not rest on the stamps, so it is
    // answered before them and reads GAP rather than UNUSABLE.
    //
    // Both answers are right, which is why the order is not arbitrary: at stream
    // start there is no window to discard either way, so nothing is lost by
    // reporting the declaration — whereas answering the stamps first would swallow
    // a declared reconnect whenever its frame happened to arrive unstamped, and
    // that one does cost a consumer a window it should have thrown away.
    {
        Beat b(rr, 40000000ull, RrInterval::Flag::DISCONTINUITY);
        EXPECT_EQ(cont(b.parser().checkContinuity(0ull)),
                  cont(RrInterval::Continuity::GAP));
        // ...and an unstamped frame declaring one is still a declared gap.
        Beat unstamped(rr, 0ull, RrInterval::Flag::DISCONTINUITY);
        EXPECT_EQ(cont(unstamped.parser().checkContinuity(40000000ull)),
                  cont(RrInterval::Continuity::GAP));
    }

    // First frame after a reconnect: a previous stamp does exist, from before the
    // drop, and the producer knows the two do not join up. DISCONTINUITY says so
    // and reads GAP even though the stamps happen to line up — the stamps cannot
    // be trusted to reveal it, because a reconnect gap can be any length,
    // including one that lands inside the budget.
    {
        const uint64_t prevUs = 40000000ull;
        Beat           b(rr, prevUs + msToUs(rr), RrInterval::Flag::DISCONTINUITY);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::GAP));
    }
}

TEST(RrIntervalContinuity, AnUnstampedFrameIsUnusableRatherThanContiguous)
{
    // The TIMESTAMP rule for a reconstruction that underflows is to emit only the
    // intervals whose instant is representable and flag the first emitted — not to
    // clamp to zero. This is what the rule protects against: were a producer to
    // stamp zero anyway, the frame must not be measurable from, because zero is
    // also what a malformed frame reports and a valid frame at zero would collide
    // with it.
    const float rr = 800.f;

    Beat b(rr, 0ull, 0u);
    EXPECT_TRUE(b.parser().isDataValid()); // the interval itself is fine
    EXPECT_EQ(b.parser().getTimestampUs(), 0ull);
    EXPECT_EQ(cont(b.parser().checkContinuity(msToUs(rr))),
              cont(RrInterval::Continuity::UNUSABLE));

    // The rule's own path: the surviving frames are stamped normally and the
    // first carries DISCONTINUITY, so a consumer gets a gap rather than a
    // fabricated instant.
    {
        const uint64_t firstUs = msToUs(rr);
        Beat           first(rr, firstUs, RrInterval::Flag::DISCONTINUITY);
        EXPECT_EQ(cont(first.parser().checkContinuity(msToUs(400.f))),
                  cont(RrInterval::Continuity::GAP));
        Beat second(rr, firstUs + msToUs(rr), 0u);
        EXPECT_EQ(cont(second.parser().checkContinuity(firstUs)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
    }
}

TEST(RrIntervalContinuity, TheAdvanceRuleAConsumerHasToFollow)
{
    // The CONSUMER OBLIGATION: the stamp passed in is the previous frame whose
    // isDataValid() held, whatever verdict that frame got. Nothing in the API can
    // enforce it and both ways of breaking it are silent, so the rule is pinned
    // here beside the two wrong readings — the shape
    // TheSubtractionAConsumerWouldWriteGetsReorderingWrong uses, for the same
    // reason: a rule nobody can see being broken has to be demonstrated.
    const float    rr = 800.f;
    const uint64_t b1 = 10000000ull;
    const uint64_t b2 = b1 + msToUs(rr);
    const uint64_t b3 = b2 + msToUs(rr); // this beat's interval is never reported
    const uint64_t b4 = b3 + msToUs(rr);

    Beat good(rr, b2, 0u);
    Beat malformed(std::numeric_limits<float>::quiet_NaN(), b3, 0u);
    Beat next(rr, b4, 0u);

    ASSERT_EQ(cont(good.parser().checkContinuity(b1)),
              cont(RrInterval::Continuity::CONTIGUOUS));
    ASSERT_EQ(cont(malformed.parser().checkContinuity(b2)),
              cont(RrInterval::Continuity::UNUSABLE));

    // Following the rule: the last frame whose isDataValid() held was `good`, at
    // b2. The pair therefore spans two beats and IS a gap — one interval went
    // unreported, and the window it falls in has a hole in it.
    EXPECT_EQ(cont(next.parser().checkContinuity(b2)),
              cont(RrInterval::Continuity::GAP));

    // Breaking it the first way — advancing across the invalid frame — carries its
    // zero stamp forward, and the gap becomes UNUSABLE: the one verdict the header
    // defines as needing no window discarded. This is the expensive direction,
    // because a window with a lost beat in it survives into an HRV figure.
    EXPECT_EQ(malformed.parser().getTimestampUs(), 0ull);
    EXPECT_EQ(cont(next.parser().checkContinuity(malformed.parser().getTimestampUs())),
              cont(RrInterval::Continuity::UNUSABLE));

    // Breaking it the second way — holding the old stamp until something reads
    // CONTIGUOUS again — never recovers, and the millisecond wrap is where it
    // shows. The residual the header states is ONE REORDERED; that is a property of
    // the rule, not of the format, which is the whole reason the rule is written
    // down.
    {
        const uint64_t pre   = 0xFFFFFFFFull * 1000ull + 600ull;
        const uint64_t after = 400ull * 1000ull; // wrapped: 400 ms past zero
        Beat wrapped(rr, after, 0u);
        ASSERT_EQ(cont(wrapped.parser().checkContinuity(pre)),
                  cont(RrInterval::Continuity::REORDERED));

        Beat following(rr, after + msToUs(rr), 0u);
        EXPECT_EQ(cont(following.parser().checkContinuity(after)),
                  cont(RrInterval::Continuity::CONTIGUOUS)) << "advanced per the rule";
        EXPECT_EQ(cont(following.parser().checkContinuity(pre)),
                  cont(RrInterval::Continuity::REORDERED)) << "held at the pre-wrap stamp";
    }
}

TEST(RrIntervalContinuity, TheBudgetDoesNotKeyOnSource)
{
    // Source is the obvious axis and the wrong one. An optical path behind a
    // batching sub-sensor is arrival-reconstructed while reading OPTICAL, and a
    // strap whose firmware stamps at the link layer is detector-stamped while
    // reading EXTERNAL — so how the stamp was obtained cuts across Source and
    // never follows it. Today no frame content moves the budget at all
    // (ThereIsExactlyOneBudgetAndNoFrameChangesIt), which makes this trivially
    // true; it is kept because a reintroduced per-class budget would be reaching
    // for exactly this field, and it would then have to fail here first.
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;
    const uint64_t offUs  = msToUs(50.f);
    const uint64_t thisUs = prevUs + 800000ull + offUs;

    const RrInterval::Source sources[] = {
        RrInterval::Source::UNKNOWN, RrInterval::Source::OPTICAL,
        RrInterval::Source::EXTERNAL, RrInterval::Source::ECG,
    };

    for (RrInterval::Source s : sources) {
        for (uint32_t flags : { 0u, uint32_t(RrInterval::Flag::DETECTOR_STAMPED) }) {
            Beat b(rr, thisUs, flags);
            b.d->mValue[RrInterval::SOURCE].u32 = static_cast<uint32_t>(s);
            EXPECT_FLOAT_EQ(b.parser().continuityToleranceMs(),
                            RrInterval::kDefaultContinuityToleranceMs)
                    << "source=" << src(s) << " flags=" << flags;
            EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                      cont(RrInterval::Continuity::CONTIGUOUS))
                    << "source=" << src(s) << " flags=" << flags;
        }
    }
}

TEST(RrIntervalContinuity, ANarrowerStampIsDeletedRatherThanWidened)
{
    // The unit confusion this class is otherwise wide open to. getTimestamp() and
    // getTimestampUs() sit next to each other, differ by a factor of 1000, and
    // both convert to uint64_t without complaint — so the wrong one yields a
    // confident GAP on a contiguous pair, silently, in the one operation the
    // contract tells consumers not to write themselves. A millisecond stamp is
    // indistinguishable at runtime from a microsecond stamp of a session 1000x
    // younger.
    static_assert(AcceptsStamp<uint64_t>::value,
                  "a microsecond stamp is what this takes");
    static_assert(!AcceptsStamp<uint32_t>::value,
                  "getTimestamp() is milliseconds and must not compile here");
    static_assert(AcceptsStampAndBudget<uint64_t>::value,
                  "the two-argument overload takes the same stamp");
    static_assert(!AcceptsStampAndBudget<uint32_t>::value,
                  "and refuses the same wrong one");

    // A stamp is an integer instant. A float one is refused for the same reason and
    // by the same route -- nothing distinguishes float ms from float us at runtime --
    // and on the two-argument form a float first argument is the arguments
    // transposed.
    static_assert(!AcceptsStamp<float>::value,
                  "a float stamp is not a microsecond instant");
    static_assert(!AcceptsStamp<double>::value,
                  "a double stamp is not a microsecond instant");
    static_assert(!AcceptsStampAndBudget<float>::value,
                  "and a float first argument is the two arguments swapped");
    static_assert(!AcceptsStampAndBudget<double>::value,
                  "and a float first argument is the two arguments swapped");
    // The other categories that converted silently before the whitelist.
    static_assert(!AcceptsStamp<UnscopedStamp>::value,
                  "an enumerator is a compile-time constant, not an instant");
    static_assert(!AcceptsStamp<int64_t>::value,
                  "a signed millisecond count is not a stamp either");
    static_assert(!AcceptsStamp<long long>::value,
                  "nor a signed one that happens to be wide enough");
    static_assert(!AcceptsStampAndBudget<UnscopedStamp>::value, "same on both overloads");
    static_assert(!AcceptsStampAndBudget<int64_t>::value, "same on both overloads");
    // A user-defined conversion reaches prevUs as silently as a built-in one.
    static_assert(!AcceptsStamp<ConvertsToStamp>::value,
                  "a class with operator uint64_t() is not a stamp either");
    static_assert(!AcceptsStampAndBudget<ConvertsToStamp>::value, "same on both overloads");

    // The transposition refused from the budget's side -- NOT every spelling of it:
    // the predicates are duals, so a type refused as a stamp is accepted as a budget
    // and the transposition survives there. See the overload's note. Pinned with the
    // legal budget spellings, since the argument for closing the closable part is
    // that it costs none of them.
    static_assert(!AcceptsBudget<uint64_t>::value,
                  "an unsigned 64-bit budget is the transposition");
    // Spelled as the width the predicate keys on, so it holds on a 32-bit target too
    // (!AcceptsBudget<size_t> would not: size_t is four bytes there).
    static_assert(!AcceptsBudget<unsigned long long>::value,
                  "however it is spelled -- and this is the spelling that pins the "
                  "width form rather than is_same<B, uint64_t>");
    static_assert(AcceptsBudget<float>::value,   "a budget is float milliseconds");
    static_assert(AcceptsBudget<double>::value,  "and a double literal still converts");
    static_assert(AcceptsBudget<int>::value,     "and an int literal: checkContinuity(x, 270)");
    static_assert(AcceptsBudget<unsigned>::value, "and 270u");
    static_assert(AcceptsBudget<long>::value,    "and a signed long, which cannot be a stamp");

    // The verdict the deleted overload would have returned, had it existed: the
    // pair below is contiguous, and a millisecond stamp reads it as a gap.
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;
    Beat           b(rr, prevUs + 800000ull, 0u);
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
              cont(RrInterval::Continuity::CONTIGUOUS));
    EXPECT_EQ(cont(b.parser().checkContinuity(static_cast<uint64_t>(prevUs / 1000ull))),
              cont(RrInterval::Continuity::GAP));
}

TEST(RrIntervalContinuity, TheSubtractionAConsumerWouldWriteGetsReorderingWrong)
{
    // Why this is an exported operation and not an exported number. Given only a
    // threshold, a consumer writes the comparison itself, and the obvious way to
    // write it is wrong: getTimestampUs() is unsigned, so on a reordered pair the
    // subtraction wraps, and converting that to float yields an enormous POSITIVE
    // gap rather than a negative one. It does not merely misclassify — it reports
    // the strongest evidence of a lost frame that the stream can produce, for a
    // pair that is only out of order. Every consumer would have to get this right
    // independently, and this is the shape they would get wrong.
    const float    rr     = 800.f;
    const uint64_t thisUs = 5000000ull;
    const uint64_t prevUs = thisUs + 400000ull;

    const float naiveDeltaMs = static_cast<float>(thisUs - prevUs) / 1000.f;
    EXPECT_GT(naiveDeltaMs, 1e12f); // ~1.8e16 ms, where the truth is -400 ms

    Beat b(rr, thisUs, 0u);
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
              cont(RrInterval::Continuity::REORDERED));
}

TEST(RrIntervalContinuity, AReorderedPairReadsReorderedNotAGiantGap)
{
    // The verdict itself, at two magnitudes of reordering. The operation does the
    // subtraction signed and in the microsecond domain; both stamps are bounded
    // by kMaxTimestampUs, so the conversion to int64_t is always in range rather
    // than relying on how an out-of-range unsigned-to-signed conversion happens
    // to behave — which C++17 leaves implementation-defined.
    const float    rr     = 800.f;
    const uint64_t thisUs = 5000000ull;

    // Well past the budget backwards.
    {
        Beat b(rr, thisUs, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(thisUs + 400000ull)),
                  cont(RrInterval::Continuity::REORDERED));
    }

    // And a pair only barely out of order: the previous stamp is one interval
    // plus a budget's worth ahead.
    {
        Beat b(rr, thisUs, 0u);
        const uint64_t prevUs = thisUs + 800000ull +
                msToUs(RrInterval::kDefaultContinuityToleranceMs + 1.f);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::REORDERED));
    }
}

TEST(RrIntervalContinuity, ADeclaredDiscontinuityOutranksTheStamps)
{
    // Perfectly spaced stamps, but the producer says it knows there is a gap —
    // a reconnect, or the first beat of a session. The producer knows better
    // than the timestamps do, and a check that papered over it would let a
    // consumer accumulate straight across the boundary.
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;

    Beat b(rr, prevUs + 800000ull, RrInterval::Flag::DISCONTINUITY);
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
              cont(RrInterval::Continuity::GAP));
    // Still distinguishable from a detected gap.
    EXPECT_TRUE(b.parser().hasDiscontinuity());
}

TEST(RrIntervalContinuity, UnusableRatherThanAFalseVerdict)
{
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;
    const uint64_t thisUs = prevUs + 800000ull;

    // A malformed frame says nothing about continuity — and must not be reported
    // as a gap, which would have a consumer discard a window for a frame that
    // never carried a beat.
    {
        Beat b(0.f, thisUs, 0u);
        EXPECT_FALSE(b.parser().isDataValid());
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::UNUSABLE));
    }

    // Zero is "not stamped" throughout this class: it is what an invalid frame
    // reports, and for that reason what the TIMESTAMP contract forbids a producer
    // to stamp. Neither end is an instant to measure from.
    {
        Beat b(rr, 0ull, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::UNUSABLE));
    }
    {
        Beat b(rr, thisUs, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(0ull)),
                  cont(RrInterval::Continuity::UNUSABLE));
    }

    // A prevUs that cannot have come from getTimestampUs(): both of Data's
    // timestamp fields are uint32 and the accessor forms ms * 1000 + us, so
    // nothing beyond that is a stamp from this API, and admitting one would put
    // the signed conversion out of range.
    {
        Beat b(rr, thisUs, 0u);
        EXPECT_EQ(cont(b.parser().checkContinuity(RrInterval::kMaxTimestampUs + 1ull)),
                  cont(RrInterval::Continuity::UNUSABLE));
    }

    // A caller-supplied budget that is not a budget. Reading NaN or a negative
    // as zero would turn every comparison into a gap.
    {
        Beat b(rr, thisUs, 0u);
        // The exception, pinned because it is one: a declared gap is answered
        // before the budget is read, so this frame reports GAP and the caller is
        // never told its budget was nonsense. That follows from the ordering being
        // deliberate — the declaration does not rest on the budget any more than it
        // rests on the stamps — but it means the guard below cannot be relied on to
        // surface a caller's bug.
        Beat declared(rr, thisUs, RrInterval::Flag::DISCONTINUITY);
        const float bad[] = { std::numeric_limits<float>::quiet_NaN(),
                              std::numeric_limits<float>::infinity(),
                              -1.f };
        for (float t : bad) {
            EXPECT_EQ(cont(b.parser().checkContinuity(prevUs, t)),
                      cont(RrInterval::Continuity::UNUSABLE)) << t;
            EXPECT_EQ(cont(declared.parser().checkContinuity(prevUs, t)),
                      cont(RrInterval::Continuity::GAP)) << t;
        }
    }
}

TEST(RrIntervalContinuity, AnIntervalTooLargeToCompareIsUnusableNotUndefined)
{
    // The one guard on rr_ms in checkContinuity() that isDataValid() does not imply,
    // and the reason it cannot be folded away as redundant: this contract passes
    // any finite positive interval through on purpose (see
    // ImplausibleButFiniteIntervalsArePassedThrough), so a decode bug can hand the
    // comparison an rr_ms far outside the range int64_t can hold. The undefined
    // operation is the float-to-int64 conversion of rr * 1000, which is undefined
    // from about 9.2e15 ms upward whether or not the multiply itself overflows —
    // the frame is still valid, so nothing upstream stops it.
    //
    // Every value below therefore has to reach a verdict rather than a conversion.
    const float tooLarge[] = {
        RrInterval::kMaxComparableRrMs,          // exactly at the bound
        1e10f,                                   // representable, but past it
        1e30f,                                   // rr * 1000 is finite (1e33) and
                                                 // still far outside int64_t
        std::numeric_limits<float>::max(),       // rr * 1000 overflows float32 too
    };

    const uint64_t prevUs = 39200000ull;
    for (float v : tooLarge) {
        Beat b(v, 40000000ull, 0u);
        EXPECT_TRUE(b.parser().isDataValid()) << v;      // a valid frame...
        EXPECT_FLOAT_EQ(b.parser().getRrMs(), v);        // ...reported as given...
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::UNUSABLE)) << v;
    }

    // And the bound is not merely large: the largest interval that IS comparable
    // still gets a real verdict, so the guard cuts where it says it does.
    {
        Beat b(std::nextafter(RrInterval::kMaxComparableRrMs, 0.f), 40000000ull, 0u);
        EXPECT_NE(cont(b.parser().checkContinuity(prevUs)),
                  cont(RrInterval::Continuity::UNUSABLE));
    }
}

TEST(RrIntervalContinuity, ACallerCanSupplyItsOwnBudget)
{
    // The escape hatch the derivation requires. The arrival-reconstructed budget
    // is the widest the lost-beat ceiling permits, not a bound on any real link
    // — nothing in this SDK bounds the connection interval — so a consumer that
    // has measured its own jitter, or a layer that knows the link parameters,
    // supplies a figure instead of accepting this one.
    const float    rr     = 800.f;
    const uint64_t prevUs = 5000000ull;
    const uint64_t thisUs = prevUs + 800000ull + msToUs(100.f);

    Beat b(rr, thisUs, 0u);

    // The default budget absorbs 100 ms...
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs)),
              cont(RrInterval::Continuity::CONTIGUOUS));
    // ...a consumer on a link it knows to be tighter than that does not have to.
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs, 50.f)),
              cont(RrInterval::Continuity::GAP));
    // A zero budget is legal and means "exact or nothing" — both halves, since a
    // boundary that excluded its own edge would make "exact" unreachable and turn
    // a zero budget into "always GAP".
    EXPECT_EQ(cont(b.parser().checkContinuity(prevUs, 0.f)),
              cont(RrInterval::Continuity::GAP));
    {
        Beat exact(rr, prevUs + 800000ull, 0u);
        EXPECT_EQ(cont(exact.parser().checkContinuity(prevUs, 0.f)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
    }
    // ...and "exact" means exact in MICROSECONDS. The 800 ms above cannot show that:
    // it is a whole number of milliseconds, so truncating rr_ms before the comparison
    // is a no-op on it. A real strap interval is a whole number of 1/1024 s ticks.
    {
        const float    tickRr   = 876.f * 125.f / 128.f; // 855.46875 ms
        const uint64_t tickRrUs = static_cast<uint64_t>(tickRr * 1000.f);
        ASSERT_NE(tickRrUs, static_cast<uint64_t>(tickRr) * 1000ull)
                << "the fixture must be fractional in milliseconds or it pins nothing";
        Beat exactUs(tickRr, prevUs + tickRrUs, 0u);
        EXPECT_EQ(cont(exactUs.parser().checkContinuity(prevUs, 0.f)),
                  cont(RrInterval::Continuity::CONTIGUOUS));
        Beat exactMsOnly(tickRr, prevUs + static_cast<uint64_t>(tickRr) * 1000ull, 0u);
        EXPECT_EQ(cont(exactMsOnly.parser().checkContinuity(prevUs, 0.f)),
                  cont(RrInterval::Continuity::REORDERED))
                << "a whole-millisecond interval is not this interval";
    }

    // A budget too large to compare is refused like any other unrepresentable one.
    // The NaN and infinity cases elsewhere never reach this bound: these are
    // finite, non-negative and pass every other test a budget faces.
    for (float t : { 1e10f, 1e30f, std::numeric_limits<float>::max(),
                     RrInterval::kMaxComparableRrMs }) {
        EXPECT_EQ(cont(b.parser().checkContinuity(prevUs, t)),
                  cont(RrInterval::Continuity::UNUSABLE)) << t;
    }
    // And the bound cuts where it says: the largest budget still comparable gets a
    // real verdict rather than falling into the same refusal.
    EXPECT_NE(cont(b.parser().checkContinuity(
                      prevUs, std::nextafter(RrInterval::kMaxComparableRrMs, 0.f))),
              cont(RrInterval::Continuity::UNUSABLE));
}

TEST(RrIntervalParser, ImplausibleButFiniteIntervalsArePassedThrough)
{
    // The parser is not an artefact filter — that policy belongs to the consumer
    // (and to ARTIFACT_SUSPECT). A 5 ms and a 40 s interval are both nonsense
    // physiologically and both parse.
    const float odd[] = { 5.f, 40000.f };

    for (float v : odd) {
        RrData m;
        m->mValue[RrInterval::RR_MS].f = v;

        RrInterval p(SDK::Sensor::DataView(*m.data(), 1));

        EXPECT_TRUE(p.isDataValid());
        EXPECT_FLOAT_EQ(p.getRrMs(), v);
        EXPECT_FLOAT_EQ(p.getBpm(), 60000.f / v);
    }
}

TEST(RrIntervalParser, BpmNeverReturnsInfinity)
{
    // Finite and positive, so the frame is valid and getRrMs() reports it — but
    // 60000/rr overflows, and an infinity is not a reading.
    RrData m;
    m->mValue[RrInterval::RR_MS].f = 1e-40f;

    RrInterval p(SDK::Sensor::DataView(*m.data(), 1));

    EXPECT_TRUE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 1e-40f);
    EXPECT_TRUE(std::isfinite(p.getBpm()));
    EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
}

TEST(RrIntervalParser, ZeroFieldFrameIsInvalid)
{
    RrData m;
    m->mValue[RrInterval::RR_MS].f = 800.f;

    // A malformed / empty frame: getters read safe zeros, never a false gap.
    RrInterval p(SDK::Sensor::DataView(*m.data(), 0));

    EXPECT_FALSE(p.isDataValid());
    EXPECT_FLOAT_EQ(p.getRrMs(), 0.f);
    EXPECT_FLOAT_EQ(p.getBpm(), 0.f);
    EXPECT_EQ(src(p.getSource()), src(RrInterval::Source::UNKNOWN));
    EXPECT_FALSE(p.hasDiscontinuity());
}
