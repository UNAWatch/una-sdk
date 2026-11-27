// Reader and writer for the raw GPS trace CSV (docs/GPS_TRACE_FORMAT.md), and
// the conversion of a trace file into a replay script for Oval::LapCounter.
//
// The watch writer and this host reader share the format. The
// writer here is for test fixtures only.
//
// Host only.
#pragma once

#include "TraceGen.hpp"

#include "Oval/Config.hpp"
#include "Oval/Sample.hpp"

#include <istream>
#include <limits>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace TraceCsv {

// Version 2: the precision column is hdop, not accuracy_m. Version
// 1 files still load, their accuracy_m divided by uereM.
constexpr int kVersion = 2;

/** The first line of every trace file, followed by the version number. */
constexpr const char* kMagic = "# trackrun-gps-trace v";

struct Row {
    Oval::Sample s;
    float       altM       = std::numeric_limits<float>::quiet_NaN();
    float       trackDistM = std::numeric_limits<float>::quiet_NaN(); // NaN when absent
    int         lap        = -1;  // -1 when absent
    std::string mode;             // pre, count, cal, pause; empty when absent
    int         lane       = -1;  // -1 with no lane column; 0 blank ("No lane")
};

struct File {
    int version = 0;
    std::vector<std::pair<std::string, std::string>> meta; // "# key=value" lines
    std::vector<Row> rows;

    /** The value of a meta key, or an empty string. */
    std::string metaValue(const std::string& key) const;
};

/** uereM converts a version 1 file's accuracy_m to HDOP. */
bool parse(std::istream& in, File& out, std::string& error, float uereM = Oval::Config().uereM);
bool read(const std::string& path, File& out, std::string& error, float uereM = Oval::Config().uereM);

void write(std::ostream& os, const File& f);
bool write(const std::string& path, const File& f);

/** The replay script for a trace file. Rows whose mode is "pre" are primed.
 *  Start is at meta start_ms, or halfway between the last pre row and the
 *  first other row. Pause and resume are halfway between the rows where the
 *  mode turns to and from "pause". A change in the lane column is a lane
 *  change before that row. The returned trace has no truth. */
TraceGen::Trace toTrace(const File& f);

/** A trace file from a generated trace, with no output columns yet. */
File fromTrace(const TraceGen::Trace& tr);

} // namespace TraceCsv
