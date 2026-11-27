// Reader and writer for the raw GPS trace CSV. See TraceCsv.hpp.

#include "TraceCsv.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace TraceCsv {

namespace {

std::string trim(const std::string& s)
{
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return std::string();
    }
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> splitCsv(const std::string& line)
{
    std::vector<std::string> out;
    std::string field;
    std::istringstream ss(line);
    while (std::getline(ss, field, ',')) {
        out.push_back(trim(field));
    }
    if (!line.empty() && line.back() == ',') {
        out.push_back(std::string());
    }
    return out;
}

bool toDouble(const std::string& s, double& v)
{
    if (s.empty()) {
        return false;
    }
    char* end = nullptr;
    v = std::strtod(s.c_str(), &end);
    return end != nullptr && *end == '\0';
}

uint32_t midpoint(uint32_t a, uint32_t b)
{
    const int32_t d = Oval::msDiff(b, a);
    return d > 0 ? a + static_cast<uint32_t>(d / 2) : a;
}

} // namespace

std::string File::metaValue(const std::string& key) const
{
    for (const auto& kv : meta) {
        if (kv.first == key) {
            return kv.second;
        }
    }
    return std::string();
}

bool parse(std::istream& in, File& out, std::string& error, float uereM)
{
    out = File();
    std::string line;

    // Version line.
    while (std::getline(in, line) && trim(line).empty()) {
    }
    const std::string magic(kMagic);
    line = trim(line);
    if (line.compare(0, magic.size(), magic) != 0) {
        error = "not a trackrun GPS trace (first line)";
        return false;
    }
    out.version = std::atoi(line.c_str() + magic.size());
    if (out.version < 1 || out.version > kVersion) {
        error = "unsupported trace version " + std::to_string(out.version);
        return false;
    }

    // Meta lines, then the column header.
    std::vector<std::string> cols;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') {
            const std::string body = trim(line.substr(1));
            const size_t eq = body.find('=');
            if (eq != std::string::npos) {
                out.meta.emplace_back(trim(body.substr(0, eq)), trim(body.substr(eq + 1)));
            }
            continue;
        }
        cols = splitCsv(line);
        break;
    }
    auto colIndex = [&](const char* name) {
        for (size_t i = 0; i < cols.size(); ++i) {
            if (cols[i] == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };
    const int cT = colIndex("t_ms"), cFix = colIndex("fix"), cLat = colIndex("lat"), cLon = colIndex("lon");
    const int cAlt = colIndex("alt_m"), cGps = colIndex("gps_dist_m");
    // Version 1 has the precision figure in metres (accuracy_m): divided
    // by uereM, it is HDOP.
    const int   cHdop   = colIndex(out.version >= 2 ? "hdop" : "accuracy_m");
    const float perHdop = out.version >= 2 ? 1.0f : uereM;
    const int cTrack = colIndex("track_dist_m"), cLap = colIndex("lap"), cMode = colIndex("mode");
    const int cLane = colIndex("lane");
    if (cT < 0 || cFix < 0 || cLat < 0 || cLon < 0) {
        error = "missing a required column (t_ms, fix, lat, lon)";
        return false;
    }

    size_t lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::vector<std::string> f = splitCsv(line);
        auto field = [&](int c) { return (c >= 0 && c < static_cast<int>(f.size())) ? f[c] : std::string(); };
        Row r;
        double v = 0.0;
        if (!toDouble(field(cT), v) || v < 0.0 || v > 4294967295.0) {
            error = "bad t_ms on data line " + std::to_string(lineNo);
            return false;
        }
        r.s.tMs = static_cast<uint32_t>(v);
        r.s.fix = field(cFix) == "1";
        r.s.lat = toDouble(field(cLat), v) ? v : 0.0;
        r.s.lon = toDouble(field(cLon), v) ? v : 0.0;
        if (toDouble(field(cAlt), v)) {
            r.altM = static_cast<float>(v);
        }
        r.s.hdop = (toDouble(field(cHdop), v) && v >= 0.0) ? static_cast<float>(v) / perHdop : -1.0f;
        if (toDouble(field(cGps), v)) {
            r.s.gpsDistM = static_cast<float>(v);
        }
        if (toDouble(field(cTrack), v)) {
            r.trackDistM = static_cast<float>(v);
        }
        if (toDouble(field(cLap), v)) {
            r.lap = static_cast<int>(v);
        }
        r.mode = field(cMode);
        if (cLane >= 0) {
            r.lane = toDouble(field(cLane), v) ? static_cast<int>(v) : 0; // blank: "No lane"
        }
        out.rows.push_back(r);
    }
    return true;
}

bool read(const std::string& path, File& out, std::string& error, float uereM)
{
    std::ifstream in(path);
    if (!in) {
        error = "cannot open " + path;
        return false;
    }
    return parse(in, out, error, uereM);
}

void write(std::ostream& os, const File& f)
{
    os << kMagic << kVersion << "\n";
    for (const auto& kv : f.meta) {
        os << "# " << kv.first << "=" << kv.second << "\n";
    }
    // A file read with no lane column is written without one.
    bool hasLane = false;
    for (const Row& r : f.rows) {
        hasLane = hasLane || r.lane >= 0;
    }
    os << "t_ms,fix,lat,lon,alt_m,hdop,gps_dist_m,track_dist_m,lap,mode" << (hasLane ? ",lane\n" : "\n");
    char buf[256];
    for (const Row& r : f.rows) {
        char alt[24] = "", hdop[24] = "", gps[24] = "", track[24] = "", lap[16] = "", lane[16] = "";
        if (std::isfinite(r.altM)) {
            std::snprintf(alt, sizeof alt, "%.1f", r.altM);
        }
        if (r.s.hdop >= 0.0f) {
            std::snprintf(hdop, sizeof hdop, "%.2f", r.s.hdop);
        }
        if (std::isfinite(r.s.gpsDistM)) {
            std::snprintf(gps, sizeof gps, "%.2f", r.s.gpsDistM);
        }
        if (std::isfinite(r.trackDistM)) {
            std::snprintf(track, sizeof track, "%.2f", r.trackDistM);
        }
        if (r.lap >= 0) {
            std::snprintf(lap, sizeof lap, "%d", r.lap);
        }
        if (r.lane > 0) {
            std::snprintf(lane, sizeof lane, "%d", r.lane);
        }
        std::snprintf(buf, sizeof buf, "%u,%d,%.7f,%.7f,%s,%s,%s,%s,%s,%s%s%s\n",
                      static_cast<unsigned>(r.s.tMs), r.s.fix ? 1 : 0, r.s.lat, r.s.lon, alt,
                      hdop, gps, track, lap, r.mode.c_str(), hasLane ? "," : "", lane);
        os << buf;
    }
}

bool write(const std::string& path, const File& f)
{
    std::ofstream os(path, std::ios::binary);
    if (!os) {
        return false;
    }
    write(os, f);
    return static_cast<bool>(os);
}

TraceGen::Trace toTrace(const File& f)
{
    TraceGen::Trace tr;
    for (const Row& r : f.rows) {
        tr.samples.push_back(r.s);
        tr.altM.push_back(r.altM);
    }

    size_t start = 0;
    while (start < f.rows.size() && f.rows[start].mode == "pre") {
        ++start;
    }
    tr.startIndex = start;

    const std::string startMeta = f.metaValue("start_ms");
    if (!startMeta.empty()) {
        tr.startMs = static_cast<uint32_t>(std::strtoul(startMeta.c_str(), nullptr, 10));
    } else if (start > 0 && start < f.rows.size()) {
        tr.startMs = midpoint(f.rows[start - 1].s.tMs, f.rows[start].s.tMs);
    } else if (!f.rows.empty()) {
        tr.startMs = f.rows[start < f.rows.size() ? start : 0].s.tMs;
    }

    const std::string laneMeta = f.metaValue("lane");
    if (!laneMeta.empty()) {
        tr.startLane = static_cast<uint8_t>(std::atoi(laneMeta.c_str()));
    } else if (start < f.rows.size() && f.rows[start].lane >= 0) {
        tr.startLane = static_cast<uint8_t>(f.rows[start].lane);
    }

    int lane = tr.startLane;
    for (size_t i = start + 1; i < f.rows.size(); ++i) {
        const Row& a = f.rows[i - 1];
        const Row& b = f.rows[i];
        const bool wasPaused = a.mode == "pause";
        const bool isPaused  = b.mode == "pause";
        if (isPaused != wasPaused) {
            TraceGen::Command c;
            c.beforeSample = i;
            c.kind = isPaused ? TraceGen::CmdKind::Pause : TraceGen::CmdKind::Resume;
            c.tMs  = midpoint(a.s.tMs, b.s.tMs);
            tr.commands.push_back(c);
        }
        if (b.lane >= 0 && b.lane != lane) {
            TraceGen::Command c;
            c.beforeSample = i;
            c.kind = TraceGen::CmdKind::SetLane;
            c.tMs  = b.s.tMs;
            c.lane = static_cast<uint8_t>(b.lane);
            tr.commands.push_back(c);
            lane = b.lane;
        }
    }
    return tr;
}

File fromTrace(const TraceGen::Trace& tr)
{
    File f;
    f.version = kVersion;
    f.meta.emplace_back("start_ms", std::to_string(tr.startMs));
    f.meta.emplace_back("lane", std::to_string(tr.startLane));

    size_t nextCmd = 0;
    int    lane    = tr.startLane;
    for (size_t i = 0; i < tr.samples.size(); ++i) {
        for (; nextCmd < tr.commands.size() && tr.commands[nextCmd].beforeSample <= i; ++nextCmd) {
            if (tr.commands[nextCmd].kind == TraceGen::CmdKind::SetLane) {
                lane = tr.commands[nextCmd].lane;
            }
        }
        Row r;
        r.s    = tr.samples[i];
        r.altM = i < tr.altM.size() ? tr.altM[i] : std::numeric_limits<float>::quiet_NaN();
        const bool paused = i < tr.pausedAt.size() && tr.pausedAt[i] != 0;
        r.mode = i < tr.startIndex ? "pre" : (paused ? "pause" : "count");
        r.lane = lane;
        f.rows.push_back(r);
    }
    return f;
}

} // namespace TraceCsv
