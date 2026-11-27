// TraceReplay.WatchTrace: a GPS trace the watch or the simulator wrote
// (docs/GPS_TRACE_FORMAT.md), replayed through Oval::Session and compared
// with the watch's own figures on its last row. Skipped unless
// TRACKRUN_TRACE names a file.

#include "OvalTestUtil.hpp"
#include "Replay.hpp"
#include "TraceCsv.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/Session.hpp"
#include "Oval/TrackStore.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

using Oval::Session;

TEST(TraceReplay, WatchTrace)
{
    const char* path = std::getenv("TRACKRUN_TRACE");
    if (path == nullptr) {
        GTEST_SKIP() << "set TRACKRUN_TRACE to a trace_*.csv the watch or simulator wrote";
    }
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::read(path, f, err)) << err;
    ASSERT_FALSE(f.rows.empty());

    // The stored tracks the run started with.
    std::string text = "trackrun-tracks v=1 count=0\n";
    size_t stored = 0;
    for (const auto& kv : f.meta) {
        if (kv.first == "store") {
            text += kv.second + "\n";
            ++stored;
        }
    }
    Oval::TrackStore store;
    const Oval::TrackStore::LoadReport rep = store.load(text.data(), text.size());
    EXPECT_EQ(rep.loaded, stored) << "every stored track line loads";

    // The simulator's precision figure, as the service scaled it.
    const std::string scale = f.metaValue("hdop_scale");
    if (!scale.empty()) {
        const float k = std::stof(scale);
        for (TraceCsv::Row& r : f.rows) {
            if (r.s.hdop >= 0.0f) {
                r.s.hdop *= k;
            }
        }
    }
    TraceGen::Trace tr = TraceCsv::toTrace(f);

    // The lane column is the Session's lane, which also changes when the run
    // joins or leaves a track by itself: the runner's own lanes come from
    // the action lines instead. (The Lap button is Run's, not the Session's.)
    tr.commands.erase(std::remove_if(tr.commands.begin(), tr.commands.end(),
                                     [](const TraceGen::Command& c) { return c.kind == TraceGen::CmdKind::SetLane; }),
                      tr.commands.end());
    std::ifstream in(path);
    std::string line;
    auto add = [&tr](TraceGen::CmdKind kind, uint32_t t, uint8_t lane) {
        size_t before = tr.samples.size();
        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
            if (Oval::msDiff(tr.samples[i].tMs, t) > 0) {
                before = i;
                break;
            }
        }
        TraceGen::Command c;
        c.beforeSample = before;
        c.kind         = kind;
        c.tMs          = t;
        c.lane         = lane;
        tr.commands.push_back(c);
    };
    while (std::getline(in, line)) {
        if (line.rfind("# lane_set=", 0) == 0) {
            // "# lane_set=<t_ms>,<lane>": 0 is "No lane".
            const std::string v = line.substr(11);
            const size_t comma = v.find(',');
            if (comma != std::string::npos) {
                add(TraceGen::CmdKind::SetLane, static_cast<uint32_t>(std::stoul(v.substr(0, comma))),
                    static_cast<uint8_t>(std::stoul(v.substr(comma + 1))));
            }
        }
    }
    std::stable_sort(tr.commands.begin(), tr.commands.end(),
                     [](const TraceGen::Command& a, const TraceGen::Command& b) { return a.beforeSample < b.beforeSample; });

    const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(),
                                                       stored > 0 ? &store : nullptr);
    // Calibration as it went (the lap it happened in, counted in the run).
    uint16_t lapsSoFar = 0;
    for (const Session::Event& e : r.events) {
        if (e.type == Session::EventType::TrackLap && e.track.type != Oval::Tracker::EventType::LapTimeRefined) {
            lapsSoFar = e.runLap;
        }
        if (e.type == Session::EventType::TrackEvent) {
            const char* what = e.track.type == Oval::Tracker::EventType::CalibrationProvisional ? "provisional"
                             : e.track.type == Oval::Tracker::EventType::CalibrationLocked      ? "locked"
                             : e.track.type == Oval::Tracker::EventType::CalibrationRejected    ? "rejected"
                                                                                                 : nullptr;
            if (what != nullptr) {
                std::printf("[ replay   ] calibration %s at %.0f s, in lap %u\n", what,
                            Oval::msDiff(e.tMs, tr.startMs) / 1000.0, lapsSoFar + 1u);
            }
        }
    }
    // The watch's own figures, on its last row.
    const TraceCsv::Row& last = f.rows.back();
    std::printf("[ replay   ] %s: %zu rows, %zu stored tracks; watch: lap %d, %.2f m; replay: %u laps, %.2f m\n",
                path, f.rows.size(), stored, last.lap, last.trackDistM, r.runLaps, r.total.back());
    EXPECT_EQ(r.decreases, 0u);
    ASSERT_GE(last.lap, 0);
    EXPECT_EQ(static_cast<int>(r.runLaps), last.lap) << "the same laps";
    EXPECT_NEAR(r.total.back(), last.trackDistM, 0.5f) << "the same total";
}
