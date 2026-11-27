/*******************************************************************************
 * @file   TrackStore.cpp
 * @brief  Oval: stored tracks and their text format. See TrackStore.hpp and
 *         docs/OVAL.md.
 *
 *         The format, one line each, every line ending in a newline:
 *
 *           trackrun-tracks v=1 count=2
 *           track v=1 n=2 lat=55.950012345 lon=-3.190045678 hdg=12.3456 r=36.5120 cw=0 alt=45.20 scale=3.100 shape=0.400 fits=3 uses=5 first=1760000000 last=1760500000 crc=1a2b3c4d
 *
 *         crc is the CRC-32 of the line up to the space before "crc=". Numbers
 *         are written and read here, in fixed decimals from integers, so the
 *         watch build needs no float printf or strtod, and a value read back is
 *         bit-identical to the one written: values are rounded to those same
 *         decimals as they enter the store.
 ******************************************************************************/

#include "Oval/TrackStore.hpp"

#include <cmath>

namespace Oval {

namespace {

constexpr double kEarthRadiusM = 6371008.8;
constexpr double kDegRad       = 0.017453292519943295;

// Decimal places kept for each value.
constexpr int kLatLonDp = 9;
constexpr int kHeadingDp = 4;
constexpr int kRadiusDp  = 4;
constexpr int kAltDp     = 2;
constexpr int kQualityDp = 3;

constexpr uint64_t kPow10i[10] = {1ull, 10ull, 100ull, 1000ull, 10000ull, 100000ull,
                                  1000000ull, 10000000ull, 100000000ull, 1000000000ull};

constexpr char kHeader[] = "trackrun-tracks";
constexpr char kTrack[]  = "track";

inline int64_t toFixed(double v, int dp) noexcept
{
    return static_cast<int64_t>(std::llround(v * static_cast<double>(kPow10i[dp])));
}

inline double fromFixed(int64_t n, int dp) noexcept
{
    return static_cast<double>(n) / static_cast<double>(kPow10i[dp]);
}

/** v rounded to dp decimals, exactly as reading it back gives it. */
inline double qd(double v, int dp) noexcept
{
    return fromFixed(toFixed(v, dp), dp);
}

inline float qf(float v, int dp) noexcept
{
    return static_cast<float>(fromFixed(toFixed(static_cast<double>(v), dp), dp));
}

/** An altitude a track can have. Anything else (a sensor's garbage) is kept
 *  as none: written as it is, its digits could be too many to read back. */
inline bool plausibleAlt(float m) noexcept
{
    return std::isfinite(m) && m > -1000.0f && m < 10000.0f;
}

inline double wrapLon(double d) noexcept
{
    while (d > 180.0) {
        d -= 360.0;
    }
    while (d < -180.0) {
        d += 360.0;
    }
    return d;
}

/** Heading in [0, 180). */
inline double wrapHeading(double h) noexcept
{
    h = std::fmod(h, 180.0);
    if (h < 0.0) {
        h += 180.0;
    }
    return h >= 180.0 ? 0.0 : h;
}

inline float clampQuality(float v) noexcept
{
    if (!(v >= 0.0f)) {
        return 0.0f;
    }
    return v > 999.0f ? 999.0f : v;
}

uint32_t crc32(const char* p, size_t n) noexcept
{
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) {
        c ^= static_cast<uint8_t>(p[i]);
        for (int k = 0; k < 8; ++k) {
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
    }
    return ~c;
}

// ---------------------------------------------------------------------------
// Writing
// ---------------------------------------------------------------------------

struct Out {
    char*  p;
    size_t cap;
    size_t n  = 0;
    bool   ok = true;

    void ch(char c) noexcept
    {
        if (n < cap) {
            p[n++] = c;
        } else {
            ok = false;
        }
    }
    void str(const char* s) noexcept
    {
        while (*s != '\0') {
            ch(*s++);
        }
    }
    void key(const char* k) noexcept
    {
        ch(' ');
        str(k);
        ch('=');
    }
    void u(uint64_t v) noexcept
    {
        char t[20];
        int  k = 0;
        do {
            t[k++] = static_cast<char>('0' + v % 10u);
            v /= 10u;
        } while (v != 0u && k < 20);
        while (k > 0) {
            ch(t[--k]);
        }
    }
    void fixed(int64_t v, int dp) noexcept
    {
        uint64_t a = 0;
        if (v < 0) {
            ch('-');
            a = static_cast<uint64_t>(-(v + 1)) + 1u;
        } else {
            a = static_cast<uint64_t>(v);
        }
        u(a / kPow10i[dp]);
        if (dp > 0) {
            ch('.');
            uint64_t f = a % kPow10i[dp];
            char     t[9];
            for (int i = dp - 1; i >= 0; --i) {
                t[i] = static_cast<char>('0' + f % 10u);
                f /= 10u;
            }
            for (int i = 0; i < dp; ++i) {
                ch(t[i]);
            }
        }
    }
    void hex8(uint32_t v) noexcept
    {
        static const char kHex[] = "0123456789abcdef";
        for (int s = 28; s >= 0; s -= 4) {
            ch(kHex[(v >> s) & 0xFu]);
        }
    }
};

// ---------------------------------------------------------------------------
// Reading
// ---------------------------------------------------------------------------

inline bool isDigit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

inline bool same(const char* s, size_t n, const char* lit) noexcept
{
    size_t i = 0;
    for (; i < n; ++i) {
        if (lit[i] == '\0' || lit[i] != s[i]) {
            return false;
        }
    }
    return lit[i] == '\0';
}

/** A decimal number, rounded to dp decimals, as an integer count of
 *  10^-dp. At most 9 integer digits. */
bool parseFixed(const char* s, size_t n, int dp, int64_t& out) noexcept
{
    size_t i   = 0;
    bool   neg = false;
    if (i < n && (s[i] == '-' || s[i] == '+')) {
        neg = s[i] == '-';
        ++i;
    }
    uint64_t ip = 0;
    int      id = 0;
    while (i < n && isDigit(s[i])) {
        if (++id > 9) {
            return false;
        }
        ip = ip * 10u + static_cast<uint64_t>(s[i] - '0');
        ++i;
    }
    uint64_t fp    = 0;
    int      fd    = 0;
    int      extra = 0;
    bool     up    = false;
    if (i < n && s[i] == '.') {
        ++i;
        while (i < n && isDigit(s[i])) {
            if (fd < dp) {
                fp = fp * 10u + static_cast<uint64_t>(s[i] - '0');
                ++fd;
            } else {
                if (extra == 0) {
                    up = s[i] >= '5'; // a later version wrote more decimals
                }
                ++extra;
            }
            if (++id > 40) {
                return false;
            }
            ++i;
        }
    }
    if (i != n || id == 0) {
        return false;
    }
    while (fd < dp) {
        fp *= 10u;
        ++fd;
    }
    const uint64_t v = ip * kPow10i[dp] + fp + (up ? 1u : 0u);
    out = neg ? -static_cast<int64_t>(v) : static_cast<int64_t>(v);
    return true;
}

bool parseUint(const char* s, size_t n, uint64_t max, uint64_t& out) noexcept
{
    if (n == 0 || n > 19) {
        return false;
    }
    uint64_t v = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!isDigit(s[i])) {
            return false;
        }
        v = v * 10u + static_cast<uint64_t>(s[i] - '0');
    }
    if (v > max) {
        return false;
    }
    out = v;
    return true;
}

bool parseHex8(const char* s, size_t n, uint32_t& out) noexcept
{
    if (n != 8) {
        return false;
    }
    uint32_t v = 0;
    for (size_t i = 0; i < n; ++i) {
        const char c = s[i];
        uint32_t   d = 0;
        if (isDigit(c)) {
            d = static_cast<uint32_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            d = static_cast<uint32_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            d = static_cast<uint32_t>(c - 'A' + 10);
        } else {
            return false;
        }
        v = (v << 4) | d;
    }
    out = v;
    return true;
}

/** Calls f(key, keyLen, value, valueLen) for each "key=value" word of a line
 *  from from on. Words without '=' are skipped. */
template <typename F>
void forEachField(const char* s, size_t n, size_t from, F&& f) noexcept
{
    size_t i = from;
    while (i < n) {
        while (i < n && s[i] == ' ') {
            ++i;
        }
        const size_t start = i;
        while (i < n && s[i] != ' ') {
            ++i;
        }
        size_t eq = start;
        while (eq < i && s[eq] != '=') {
            ++eq;
        }
        if (eq < i && eq > start) {
            f(s + start, eq - start, s + eq + 1, i - eq - 1);
        }
    }
}

/** The first word of a line. */
inline size_t firstWord(const char* s, size_t n) noexcept
{
    size_t i = 0;
    while (i < n && s[i] != ' ') {
        ++i;
    }
    return i;
}

} // namespace

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

TrackModel TrackRecord::model() const noexcept
{
    return TrackModel::fromGeo(lat, lon, headingDeg, kerbRadiusM, clockwise, altM);
}

TrackStore::TrackStore(const Config& cfg) noexcept
{
    setConfig(cfg);
    clear();
}

void TrackStore::setConfig(const Config& cfg) noexcept
{
    mNearM    = cfg.storeNearM;
    mBlendMax = cfg.storeBlendMaxWeight > 0 ? cfg.storeBlendMaxWeight : 1;
}

void TrackStore::clear() noexcept
{
    for (TrackRecord& r : mRecs) {
        r = TrackRecord();
    }
}

uint8_t TrackStore::count() const noexcept
{
    uint8_t n = 0;
    for (const TrackRecord& r : mRecs) {
        n = static_cast<uint8_t>(n + (r.isValid() ? 1 : 0));
    }
    return n;
}

TrackRecord* TrackStore::slot(uint8_t number) noexcept
{
    if (number == 0 || number > kMaxTracks || !mRecs[number - 1].isValid()) {
        return nullptr;
    }
    return &mRecs[number - 1];
}

const TrackRecord* TrackStore::find(uint8_t number) const noexcept
{
    if (number == 0 || number > kMaxTracks || !mRecs[number - 1].isValid()) {
        return nullptr;
    }
    return &mRecs[number - 1];
}

const TrackRecord* TrackStore::findNear(double lat, double lon, float* distM, uint8_t skip) const noexcept
{
    const TrackRecord* best  = nullptr;
    double             bestD = 0.0;
    for (const TrackRecord& r : mRecs) {
        if (!r.isValid() || (skip != 0 && r.number == skip)) {
            continue;
        }
        const double dy = (lat - r.lat) * kDegRad * kEarthRadiusM;
        const double dx = wrapLon(lon - r.lon) * kDegRad * kEarthRadiusM * std::cos(0.5 * (lat + r.lat) * kDegRad);
        const double d  = std::sqrt(dx * dx + dy * dy);
        if (d <= static_cast<double>(mNearM) && (best == nullptr || d < bestD)) {
            best  = &r;
            bestD = d;
        }
    }
    if (best != nullptr && distM != nullptr) {
        *distM = static_cast<float>(bestD);
    }
    return best;
}

bool TrackStore::fill(TrackRecord& r, const TrackModel& m, const FitQuality& q) const noexcept
{
    if (!m.isValid() || !m.hasGeo() || !std::isfinite(m.centreLat()) || !std::isfinite(m.centreLon())) {
        return false;
    }
    r.lat         = qd(m.centreLat(), kLatLonDp);
    r.lon         = qd(wrapLon(m.centreLon()), kLatLonDp);
    r.headingDeg  = static_cast<float>(wrapHeading(qd(wrapHeading(m.headingDeg()), kHeadingDp)));
    r.kerbRadiusM = qf(m.kerbRadiusM(), kRadiusDp);
    r.altM        = plausibleAlt(m.altitudeM()) ? qf(m.altitudeM(), kAltDp)
                                                 : std::numeric_limits<float>::quiet_NaN();
    r.clockwise   = m.clockwise();
    r.scaleM      = qf(clampQuality(q.scaleM), kQualityDp);
    r.shapeM      = qf(clampQuality(q.shapeM), kQualityDp);
    r.version     = kFormatVersion;
    // Only what load() takes back (a fit beyond its bounds would be saved,
    // then dropped at the next start).
    return r.lat >= -90.0 && r.lat <= 90.0 && r.kerbRadiusM >= 30.0f && r.kerbRadiusM <= 45.0f;
}

TrackStore::SaveResult TrackStore::saveNew(const TrackModel& model, const FitQuality& q, uint32_t nowS,
                                           uint16_t keep) noexcept
{
    SaveResult res;
    TrackRecord rec;
    if (!fill(rec, model, q)) {
        return res;
    }
    // The lowest free number; when full, the least recently used track goes.
    int target = -1;
    for (int i = 0; i < kMaxTracks; ++i) {
        if (!mRecs[i].isValid()) {
            target = i;
            break;
        }
    }
    if (target < 0) {
        for (int i = 0; i < kMaxTracks; ++i) {
            if ((keep >> (mRecs[i].number - 1)) & 1u) {
                continue;
            }
            const TrackRecord& a = mRecs[i];
            if (target < 0) {
                target = i;
                continue;
            }
            const TrackRecord& b = mRecs[target];
            if (a.lastUsedS < b.lastUsedS || (a.lastUsedS == b.lastUsedS && a.firstSavedS < b.firstSavedS)) {
                target = i;
            }
        }
        if (target < 0) {
            return res; // every track kept: none to replace
        }
        res.replaced = mRecs[target].number;
    }
    rec.number      = static_cast<uint8_t>(target + 1);
    rec.fits        = 1;
    rec.uses        = 1;
    rec.firstSavedS = nowS;
    rec.lastUsedS   = nowS;
    mRecs[target]   = rec;
    res.number      = rec.number;
    return res;
}

bool TrackStore::update(uint8_t number, const TrackModel& model, const FitQuality& q, uint32_t nowS) noexcept
{
    TrackRecord* r = slot(number);
    TrackRecord  rec;
    if (r == nullptr || !fill(rec, model, q)) {
        return false;
    }
    rec.number      = r->number;
    rec.firstSavedS = r->firstSavedS;
    rec.uses        = r->uses;
    rec.fits        = 1;
    *r = rec;
    return touch(number, nowS);
}

bool TrackStore::blend(uint8_t number, const TrackModel& model, const FitQuality& q, uint32_t nowS) noexcept
{
    TrackRecord* r = slot(number);
    TrackRecord  v;
    if (r == nullptr || !fill(v, model, q)) {
        return false;
    }
    const uint16_t fits = r->fits > 0 ? r->fits : 1;
    const double   w    = static_cast<double>(fits < mBlendMax ? fits : mBlendMax);
    const double   k    = 1.0 / (w + 1.0); // the visit's share

    r->lat = qd(r->lat + k * (v.lat - r->lat), kLatLonDp);
    r->lon = qd(wrapLon(r->lon + k * wrapLon(v.lon - r->lon)), kLatLonDp);

    // Headings are the same half a turn apart.
    double dh = std::fmod(static_cast<double>(v.headingDeg) - r->headingDeg, 180.0);
    if (dh > 90.0) {
        dh -= 180.0;
    } else if (dh <= -90.0) {
        dh += 180.0;
    }
    r->headingDeg = static_cast<float>(wrapHeading(qd(wrapHeading(r->headingDeg + k * dh), kHeadingDp)));
    r->kerbRadiusM = qf(static_cast<float>(r->kerbRadiusM + k * (v.kerbRadiusM - r->kerbRadiusM)), kRadiusDp);
    if (std::isfinite(r->altM) && plausibleAlt(v.altM)) {
        r->altM = qf(static_cast<float>(r->altM + k * (v.altM - r->altM)), kAltDp);
    } else if (plausibleAlt(v.altM)) {
        r->altM = v.altM;
    }
    r->scaleM    = qf(static_cast<float>(r->scaleM + k * (v.scaleM - r->scaleM)), kQualityDp);
    r->shapeM    = qf(static_cast<float>(r->shapeM + k * (v.shapeM - r->shapeM)), kQualityDp);
    r->clockwise = v.clockwise;
    r->version   = kFormatVersion;
    if (r->fits < 0xFFFFu) {
        ++r->fits;
    }
    return touch(number, nowS);
}

bool TrackStore::touch(uint8_t number, uint32_t nowS) noexcept
{
    TrackRecord* r = slot(number);
    if (r == nullptr) {
        return false;
    }
    if (r->uses < 0xFFFFu) {
        ++r->uses;
    }
    r->lastUsedS = nowS;
    return true;
}

bool TrackStore::forget(uint8_t number) noexcept
{
    TrackRecord* r = slot(number);
    if (r == nullptr) {
        return false;
    }
    *r = TrackRecord();
    return true;
}

uint8_t TrackStore::list(const TrackRecord* out[kMaxTracks]) const noexcept
{
    uint8_t n = 0;
    for (const TrackRecord& r : mRecs) {
        if (r.isValid()) {
            out[n++] = &r;
        }
    }
    return n;
}

void TrackStore::applyRun(const StoreOutcome* outcomes, const uint8_t* suppliedNumbers, uint8_t n, uint32_t nowS,
                          Applied* out) noexcept
{
    // The stored tracks the run used are kept.
    uint16_t keep = 0;
    for (uint8_t i = 0; i < n; ++i) {
        if (suppliedNumbers[i] >= 1 && suppliedNumbers[i] <= kMaxTracks) {
            keep = static_cast<uint16_t>(keep | (1u << (suppliedNumbers[i] - 1)));
        }
    }
    for (int pass = 0; pass < 2; ++pass) {
        for (uint8_t i = 0; i < n; ++i) {
            const bool isNew = outcomes[i].kind == StoreOutcome::Kind::New;
            if (isNew == (pass == 1)) {
                out[i] = apply(outcomes[i], suppliedNumbers[i], nowS, keep);
                // And a new one saved, also one an update made new (its
                // stored track gone): a later new one must not take its slot.
                if (out[i].kind == StoreOutcome::Kind::New && out[i].number >= 1 && out[i].number <= kMaxTracks) {
                    keep = static_cast<uint16_t>(keep | (1u << (out[i].number - 1)));
                }
            }
        }
    }
}

TrackStore::Applied TrackStore::apply(const StoreOutcome& o, uint8_t suppliedNumber, uint32_t nowS,
                                      uint16_t keep) noexcept
{
    Applied a;
    const bool haveSupplied = find(suppliedNumber) != nullptr;
    StoreOutcome::Kind kind = o.kind;
    // Updated, Replaced and Unchanged refer to the supplied track; if it has
    // gone meanwhile, a locked fit is still worth keeping.
    if (!haveSupplied && (kind == StoreOutcome::Kind::Updated || kind == StoreOutcome::Kind::Replaced)) {
        kind = StoreOutcome::Kind::New;
    }
    switch (kind) {
    case StoreOutcome::Kind::None:
        break;
    case StoreOutcome::Kind::New: {
        const SaveResult s = saveNew(o.model, o.quality, nowS, keep);
        a.number   = s.number;
        a.replaced = s.replaced;
        a.changed  = s.number != 0;
        break;
    }
    case StoreOutcome::Kind::Updated:
        a.changed = blend(suppliedNumber, o.model, o.quality, nowS);
        a.number  = a.changed ? suppliedNumber : 0;
        break;
    case StoreOutcome::Kind::Replaced:
        a.changed = update(suppliedNumber, o.model, o.quality, nowS);
        a.number  = a.changed ? suppliedNumber : 0;
        break;
    case StoreOutcome::Kind::Unchanged:
        a.changed = touch(suppliedNumber, nowS);
        a.number  = a.changed ? suppliedNumber : 0;
        break;
    }
    a.kind = a.changed ? kind : StoreOutcome::Kind::None;
    if (a.changed && o.lane != 0) {
        TrackRecord* r = slot(a.number);
        if (r != nullptr && r->isValid()) {
            r->lane = o.lane;
        }
    }
    return a;
}

// ---------------------------------------------------------------------------
// Text format
// ---------------------------------------------------------------------------

size_t TrackStore::serialise(char* buf, size_t cap) const noexcept
{
    if (buf == nullptr) {
        return 0;
    }
    Out o{buf, cap};
    o.str(kHeader);
    o.key("v");
    o.u(kFormatVersion);
    o.key("count");
    o.u(count());
    o.ch('\n');
    for (const TrackRecord& r : mRecs) {
        if (!r.isValid()) {
            continue;
        }
        const size_t start = o.n;
        o.str(kTrack);
        o.key("v");
        o.u(kFormatVersion);
        o.key("n");
        o.u(r.number);
        o.key("lat");
        o.fixed(toFixed(r.lat, kLatLonDp), kLatLonDp);
        o.key("lon");
        o.fixed(toFixed(r.lon, kLatLonDp), kLatLonDp);
        o.key("hdg");
        o.fixed(toFixed(r.headingDeg, kHeadingDp), kHeadingDp);
        o.key("r");
        o.fixed(toFixed(r.kerbRadiusM, kRadiusDp), kRadiusDp);
        o.key("cw");
        o.u(r.clockwise ? 1u : 0u);
        o.key("alt");
        if (std::isfinite(r.altM)) {
            o.fixed(toFixed(r.altM, kAltDp), kAltDp);
        } else {
            o.str("nan");
        }
        o.key("scale");
        o.fixed(toFixed(r.scaleM, kQualityDp), kQualityDp);
        o.key("shape");
        o.fixed(toFixed(r.shapeM, kQualityDp), kQualityDp);
        o.key("fits");
        o.u(r.fits);
        o.key("uses");
        o.u(r.uses);
        o.key("first");
        o.u(r.firstSavedS);
        o.key("last");
        o.u(r.lastUsedS);
        if (r.lane != 0) {
            // The lane last run there, which the run takes when it joins
            // the track by itself. Optional: readers that do not know it
            // skip it.
            o.key("ln");
            o.u(r.lane);
        }
        if (!o.ok) {
            return 0;
        }
        const uint32_t crc = crc32(buf + start, o.n - start);
        o.key("crc");
        o.hex8(crc);
        o.ch('\n');
    }
    return o.ok ? o.n : 0;
}

TrackStore::LoadReport TrackStore::load(const char* text, size_t len) noexcept
{
    LoadReport rep;
    clear();
    if (text == nullptr) {
        return rep;
    }
    size_t pos   = 0;
    bool   first = true;
    while (pos < len) {
        size_t end = pos;
        while (end < len && text[end] != '\n') {
            ++end;
        }
        const bool   complete = end < len;
        const char*  s        = text + pos;
        size_t       n        = end - pos;
        pos = complete ? end + 1 : len;
        ++rep.lines;
        if (n > 0 && s[n - 1] == '\r') {
            --n;
        }
        const bool isFirst = first;
        first = false;

        const size_t w = firstWord(s, n);
        if (!complete) {
            // A cut-off file: the last line has no end, so it cannot be
            // trusted (a cut number still parses).
            if (n > 0) {
                ++rep.truncated;
            }
            break;
        }
        if (isFirst && same(s, w, kHeader)) {
            bool beyond = false; // a version too large for the report's field: newer all the same
            forEachField(s, n, w, [&](const char* k, size_t kn, const char* v, size_t vn) {
                uint64_t u = 0;
                if (same(k, kn, "v") && parseUint(v, vn, 255, u)) {
                    rep.version = static_cast<uint8_t>(u);
                } else if (same(k, kn, "v") && vn > 0) {
                    // Digits past 255 (any number of them): a later version.
                    bool digits = true;
                    for (size_t i = 0; i < vn; ++i) {
                        digits = digits && isDigit(v[i]);
                    }
                    beyond = beyond || digits;
                } else if (same(k, kn, "count") && parseUint(v, vn, 255, u)) {
                    rep.expected = static_cast<uint8_t>(u);
                }
            });
            rep.headerOk     = rep.version > 0 || beyond;
            rep.newerVersion = rep.version > kFormatVersion || beyond;
            continue;
        }
        if (!same(s, w, kTrack) || n > kMaxLineBytes) {
            ++rep.otherLines;
            continue;
        }

        // The CRC covers the line up to the space before "crc=".
        size_t c = n;
        for (size_t i = n; i >= 5; --i) {
            if (same(s + i - 5, 5, " crc=")) {
                c = i - 5;
                break;
            }
        }
        uint32_t want = 0;
        if (c == n || !parseHex8(s + c + 5, n - c - 5, want) || crc32(s, c) != want) {
            ++rep.badChecksum;
            continue;
        }

        TrackRecord r;
        uint32_t    have = 0;
        bool        bad  = false;
        constexpr uint32_t kN = 1, kLat = 2, kLon = 4, kHdg = 8, kR = 16, kCw = 32;
        forEachField(s, c, w, [&](const char* k, size_t kn, const char* v, size_t vn) {
            int64_t  f = 0;
            uint64_t u = 0;
            if (same(k, kn, "v")) {
                bad |= !parseUint(v, vn, 255, u);
                r.version = static_cast<uint8_t>(u);
            } else if (same(k, kn, "n")) {
                bad |= !parseUint(v, vn, kMaxTracks, u) || u == 0;
                r.number = static_cast<uint8_t>(u);
                have |= kN;
            } else if (same(k, kn, "lat")) {
                bad |= !parseFixed(v, vn, kLatLonDp, f);
                r.lat = fromFixed(f, kLatLonDp);
                bad |= !(r.lat >= -90.0 && r.lat <= 90.0);
                have |= kLat;
            } else if (same(k, kn, "lon")) {
                bad |= !parseFixed(v, vn, kLatLonDp, f);
                r.lon = fromFixed(f, kLatLonDp);
                bad |= !(r.lon >= -180.0 && r.lon <= 180.0);
                have |= kLon;
            } else if (same(k, kn, "hdg")) {
                bad |= !parseFixed(v, vn, kHeadingDp, f);
                r.headingDeg = static_cast<float>(fromFixed(f, kHeadingDp));
                bad |= !(r.headingDeg >= 0.0f && r.headingDeg < 180.0f);
                have |= kHdg;
            } else if (same(k, kn, "r")) {
                bad |= !parseFixed(v, vn, kRadiusDp, f);
                r.kerbRadiusM = static_cast<float>(fromFixed(f, kRadiusDp));
                bad |= !(r.kerbRadiusM >= 30.0f && r.kerbRadiusM <= 45.0f);
                have |= kR;
            } else if (same(k, kn, "cw")) {
                bad |= !parseUint(v, vn, 1, u);
                r.clockwise = u == 1u;
                have |= kCw;
            } else if (same(k, kn, "alt")) {
                if (same(v, vn, "nan")) {
                    r.altM = std::numeric_limits<float>::quiet_NaN();
                } else {
                    bad |= !parseFixed(v, vn, kAltDp, f);
                    r.altM = static_cast<float>(fromFixed(f, kAltDp));
                }
            } else if (same(k, kn, "scale")) {
                bad |= !parseFixed(v, vn, kQualityDp, f) || f < 0;
                r.scaleM = static_cast<float>(fromFixed(f, kQualityDp));
            } else if (same(k, kn, "shape")) {
                bad |= !parseFixed(v, vn, kQualityDp, f) || f < 0;
                r.shapeM = static_cast<float>(fromFixed(f, kQualityDp));
            } else if (same(k, kn, "fits")) {
                bad |= !parseUint(v, vn, 0xFFFFu, u);
                r.fits = static_cast<uint16_t>(u);
            } else if (same(k, kn, "uses")) {
                bad |= !parseUint(v, vn, 0xFFFFu, u);
                r.uses = static_cast<uint16_t>(u);
            } else if (same(k, kn, "first")) {
                bad |= !parseUint(v, vn, 0xFFFFFFFFu, u);
                r.firstSavedS = static_cast<uint32_t>(u);
            } else if (same(k, kn, "last")) {
                bad |= !parseUint(v, vn, 0xFFFFFFFFu, u);
                r.lastUsedS = static_cast<uint32_t>(u);
            } else if (same(k, kn, "ln")) {
                bad |= !parseUint(v, vn, 8u, u);
                r.lane = static_cast<uint8_t>(u);
            }
            // Other keys: from a later version; skipped.
        });
        if (bad || have != (kN | kLat | kLon | kHdg | kR | kCw)) {
            ++rep.badField;
            continue;
        }
        if (r.fits == 0) {
            r.fits = 1;
        }
        if (r.version == 0) {
            r.version = rep.version;
        }
        if (mRecs[r.number - 1].isValid()) {
            ++rep.duplicate;
            continue;
        }
        mRecs[r.number - 1] = r;
        ++rep.loaded;
    }
    return rep;
}

} // namespace Oval
