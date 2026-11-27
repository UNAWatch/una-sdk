// The GPS trace CSV (docs/GPS_TRACE_FORMAT.md): reading, writing, and
// replaying a trace file through Oval::LapCounter.
//
// The fixture in data/ was made by the trace generator (DISABLED_WriteFixture
// below) and stands in for a real trace. Its output
// columns were written by this LapCounter, so the replay test is also a
// regression check: if counting changes on purpose, regenerate the fixture.

#include "OvalTestUtil.hpp"
#include "TraceCsv.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/LapCounter.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <sstream>
#include <string>

namespace {

const std::string kFixtureV1 = std::string(TRACE_DATA_DIR) + "/trace_fixture_v1.csv";
const std::string kFixtureV2 = std::string(TRACE_DATA_DIR) + "/trace_fixture_v2.csv";

// Laps completed after each reading, from the events.
std::vector<int> lapsPerReading(const Replay::Result& r, size_t readings)
{
    std::vector<int> laps(readings, 0);
    size_t k = 0;
    int count = 0;
    for (size_t i = 0; i < readings; ++i) {
        while (k < r.eventSample.size() && r.eventSample[k] == i) {
            ++count;
            ++k;
        }
        laps[i] = count;
    }
    return laps;
}

TraceCsv::File fixtureFile()
{
    TraceGen::Generator g(TraceGen::Track(), OvalTest::noisy(3.0f), 2026);
    g.begin(0.1f, 2);
    g.runLaps(3, OvalTest::runner());
    g.run(40.0, OvalTest::runner());
    // The outputs come from replaying the file as written, with its rounded
    // inputs, as the watch's would come from the readings it logged.
    std::stringstream ss;
    TraceCsv::write(ss, TraceCsv::fromTrace(g.finish()));
    TraceCsv::File f;
    std::string err;
    TraceCsv::parse(ss, f, err);
    const TraceGen::Trace  tr   = TraceCsv::toTrace(f);
    const Replay::Result   r    = Replay::run(tr);
    const std::vector<int> laps = lapsPerReading(r, tr.samples.size());

    f.meta.emplace_back("source", "synthetic: TraceGen seed 2026, lane 2, 3 laps at 5 m/s, 3 m noise");
    for (size_t i = tr.startIndex; i < f.rows.size(); ++i) {
        f.rows[i].trackDistM = r.total[i];
        f.rows[i].lap        = laps[i];
    }
    return f;
}

} // namespace

// Writes the version 2 fixture. The version 1 fixture (accuracy_m in metres)
// stays as it is: version 1 files must keep loading.
TEST(TraceReplay, DISABLED_WriteFixture)
{
    ASSERT_TRUE(TraceCsv::write(kFixtureV2, fixtureFile()));
}

namespace {

void replayFixture(const std::string& path, int version)
{
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::read(path, f, err)) << err;
    EXPECT_EQ(f.version, version);
    EXPECT_FALSE(f.metaValue("start_ms").empty());
    EXPECT_EQ(f.metaValue("lane"), "2");
    ASSERT_GT(f.rows.size(), 200u);

    const TraceGen::Trace tr = TraceCsv::toTrace(f);
    EXPECT_EQ(tr.startLane, 2);
    EXPECT_GT(tr.startIndex, 0u);
    const Replay::Result r = Replay::run(tr);
    EXPECT_EQ(r.decreases, 0u);

    // Three laps of lane 2, ending exactly.
    ASSERT_EQ(r.events.size(), 3u);
    EXPECT_NEAR(r.events.back().totalM, 3.0f * Oval::LaneTable::outdoor().lapLengthM(2), 0.01f);
    EXPECT_EQ(f.rows.back().lap, 3);

    // The recorded output columns match this replay.
    const std::vector<int> laps = lapsPerReading(r, tr.samples.size());
    for (size_t i = tr.startIndex; i < f.rows.size(); ++i) {
        EXPECT_NEAR(f.rows[i].trackDistM, r.total[i], 0.01f) << "row " << i;
        EXPECT_EQ(f.rows[i].lap, laps[i]) << "row " << i;
    }
}

} // namespace

TEST(TraceReplay, ReplaysTheFixture)
{
    // Version 1: accuracy_m (metres) is read as HDOP, divided by uereM. The
    // first row's 3.7 m is HDOP 0.925, an error of 3.7 m again, so the start
    // point's weights, and the replay, match a reading in metres.
    replayFixture(kFixtureV1, 1);
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::read(kFixtureV1, f, err)) << err;
    ASSERT_FALSE(f.rows.empty());
    EXPECT_FLOAT_EQ(f.rows[0].s.hdop, 3.7f / 4.0f);
    EXPECT_FLOAT_EQ(Oval::hdopErrorM(f.rows[0].s.hdop, Oval::Config()), 3.7f);
}

TEST(TraceReplay, ReplaysTheVersion2Fixture)
{
    replayFixture(kFixtureV2, 2);
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::read(kFixtureV2, f, err)) << err;
    for (const auto& r : f.rows) {
        EXPECT_GE(r.s.hdop, 0.7f);
        EXPECT_LE(r.s.hdop, 5.0f);
    }
}

TEST(TraceReplay, RoundTripsAGeneratedTrace)
{
    // A run with a pause and a lane change, written out and read back.
    TraceGen::Generator g(TraceGen::Track(), OvalTest::noisy(3.0f), 99);
    g.begin(0.3f, 3);
    g.runLaps(1.4, OvalTest::runner());
    g.pause();
    g.stand(20.0);
    g.resume();
    g.changeLane(1);
    g.run(2.6 * 400.0, OvalTest::runner());
    g.run(40.0, OvalTest::runner());
    const TraceGen::Trace tr = g.finish();

    std::stringstream ss;
    TraceCsv::write(ss, TraceCsv::fromTrace(tr));
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::parse(ss, f, err)) << err;
    ASSERT_EQ(f.rows.size(), tr.samples.size());

    for (size_t i = 0; i < tr.samples.size(); ++i) {
        const Oval::Sample& a = tr.samples[i];
        const Oval::Sample& b = f.rows[i].s;
        EXPECT_EQ(a.tMs, b.tMs);
        EXPECT_EQ(a.fix, b.fix);
        EXPECT_NEAR(a.lat, b.lat, 1e-7);
        EXPECT_NEAR(a.lon, b.lon, 1e-7);
        EXPECT_NEAR(a.hdop, b.hdop, 0.005f); // written to 0.01
        EXPECT_EQ(std::isfinite(a.gpsDistM), std::isfinite(b.gpsDistM));
        if (std::isfinite(a.gpsDistM)) {
            EXPECT_NEAR(a.gpsDistM, b.gpsDistM, 0.006f); // written to 0.01 m
        }
    }

    const TraceGen::Trace back = TraceCsv::toTrace(f);
    EXPECT_EQ(back.startIndex, tr.startIndex);
    EXPECT_EQ(back.startMs, tr.startMs);
    EXPECT_EQ(back.startLane, 3);
    ASSERT_EQ(back.commands.size(), 3u); // pause, resume, lane change
    EXPECT_EQ(back.commands[0].kind, TraceGen::CmdKind::Pause);
    EXPECT_EQ(back.commands[1].kind, TraceGen::CmdKind::Resume);
    EXPECT_EQ(back.commands[2].kind, TraceGen::CmdKind::SetLane);
    EXPECT_EQ(back.commands[2].lane, 1);

    // Pause and resume land between readings rather than at the exact
    // moment, so only the lap count and the credited distance must agree.
    const Replay::Result direct = Replay::run(tr);
    const Replay::Result viaCsv = Replay::run(back);
    ASSERT_EQ(direct.events.size(), tr.crossMs.size());
    ASSERT_FALSE(direct.events.empty());
    ASSERT_EQ(direct.events.size(), viaCsv.events.size());
    EXPECT_NEAR(direct.events.back().totalM, viaCsv.events.back().totalM, 0.5f);
    EXPECT_EQ(viaCsv.decreases, 0u);
}

TEST(TraceReplay, RejectsOtherFilesAndVersions)
{
    TraceCsv::File f;
    std::string err;
    {
        std::istringstream in("t_ms,fix,lat,lon\n1,1,55.9,-3.1\n");
        EXPECT_FALSE(TraceCsv::parse(in, f, err));
    }
    {
        // Version 2 is known; 3 is newer than this reader.
        std::istringstream in("# trackrun-gps-trace v3\nt_ms,fix,lat,lon\n1,1,55.9,-3.1\n");
        EXPECT_FALSE(TraceCsv::parse(in, f, err));
        EXPECT_NE(err.find("version"), std::string::npos);
    }
    {
        std::istringstream in("# trackrun-gps-trace v1\nt_ms,fix,lat\n1,1,55.9\n");
        EXPECT_FALSE(TraceCsv::parse(in, f, err));
    }
    {
        std::istringstream in("# trackrun-gps-trace v1\nt_ms,fix,lat,lon\nabc,1,55.9,-3.1\n");
        EXPECT_FALSE(TraceCsv::parse(in, f, err));
    }
}

TEST(TraceReplay, ReadsColumnsByNameWithBlanksAndExtras)
{
    // Version 1: accuracy_m is metres, read as HDOP by dividing by uereM
    // (4 m), so 3.5 m is HDOP 0.875.
    std::istringstream in(
        "# trackrun-gps-trace v1\n"
        "# lane=4\n"
        "# note = written by hand\n"
        "lon,lat,t_ms,extra,fix,gps_dist_m,accuracy_m,mode\n"
        "-3.19,55.95,4294966000,x,1,,3.5,pre\n"
        "-3.19001,55.95001,4294966999,y,0,12.25,,count\n");
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::parse(in, f, err)) << err;
    EXPECT_EQ(f.metaValue("lane"), "4");
    EXPECT_EQ(f.metaValue("note"), "written by hand");
    ASSERT_EQ(f.rows.size(), 2u);
    EXPECT_EQ(f.rows[0].s.tMs, 4294966000u);
    EXPECT_TRUE(f.rows[0].s.fix);
    EXPECT_DOUBLE_EQ(f.rows[0].s.lat, 55.95);
    EXPECT_DOUBLE_EQ(f.rows[0].s.lon, -3.19);
    EXPECT_TRUE(std::isnan(f.rows[0].s.gpsDistM));
    EXPECT_FLOAT_EQ(f.rows[0].s.hdop, 3.5f / 4.0f);
    EXPECT_FALSE(f.rows[1].s.fix);
    EXPECT_FLOAT_EQ(f.rows[1].s.gpsDistM, 12.25f);
    EXPECT_FLOAT_EQ(f.rows[1].s.hdop, -1.0f); // blank: unknown
    EXPECT_TRUE(std::isnan(f.rows[1].trackDistM));
    EXPECT_EQ(f.rows[1].lap, -1);

    const TraceGen::Trace tr = TraceCsv::toTrace(f);
    EXPECT_EQ(tr.startIndex, 1u);
    EXPECT_EQ(tr.startLane, 4);
    EXPECT_EQ(tr.startMs, 4294966000u + 499u); // halfway between the pre row and the next

    // The same converted with another uereM.
    std::istringstream again(
        "# trackrun-gps-trace v1\nt_ms,fix,lat,lon,accuracy_m\n1000,1,55.95,-3.19,3.5\n");
    ASSERT_TRUE(TraceCsv::parse(again, f, err, 5.0f)) << err;
    EXPECT_FLOAT_EQ(f.rows[0].s.hdop, 0.7f);
}

TEST(TraceReplay, Version2ReadsHdopAsItIs)
{
    // Version 2: the hdop column, as it is. accuracy_m is not a column
    // of this version, so it is ignored like any unknown column.
    std::istringstream in(
        "# trackrun-gps-trace v2\n"
        "t_ms,fix,lat,lon,hdop,accuracy_m\n"
        "1000,1,55.95,-3.19,1.30,9.9\n"
        "2000,1,55.95,-3.19,,9.9\n"
        "3000,1,55.95,-3.19,4.75,\n");
    TraceCsv::File f;
    std::string err;
    ASSERT_TRUE(TraceCsv::parse(in, f, err)) << err;
    EXPECT_EQ(f.version, 2);
    ASSERT_EQ(f.rows.size(), 3u);
    EXPECT_FLOAT_EQ(f.rows[0].s.hdop, 1.3f);
    EXPECT_FLOAT_EQ(f.rows[1].s.hdop, -1.0f); // blank: unknown
    EXPECT_FLOAT_EQ(f.rows[2].s.hdop, 4.75f);

    // The writer leaves an unknown HDOP blank, and writes version 2.
    std::stringstream ss;
    TraceCsv::write(ss, f);
    const std::string text = ss.str();
    EXPECT_EQ(text.rfind("# trackrun-gps-trace v2\n", 0), 0u);
    EXPECT_NE(text.find("t_ms,fix,lat,lon,alt_m,hdop,gps_dist_m"), std::string::npos);
    EXPECT_NE(text.find("2000,1,55.9500000,-3.1900000,,,"), std::string::npos);
    EXPECT_NE(text.find(",4.75,"), std::string::npos);
}
