/**
 ******************************************************************************
 * @file    WorkoutLibrary.cpp
 * @brief   The structured workouts on the watch, as a list to choose from.
 ******************************************************************************
 */

#include "SDK/Workout/WorkoutLibrary.hpp"

#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace SDK::Workout {

namespace {

char lower(char c)
{
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

bool hasFitExtension(const char* name, size_t len)
{
    static constexpr char kExt[] = ".fit";
    constexpr size_t      kExtLen = sizeof(kExt) - 1;
    if (len <= kExtLen) {
        return false;
    }
    for (size_t i = 0; i < kExtLen; ++i) {
        if (lower(name[len - kExtLen + i]) != kExt[i]) {
            return false;
        }
    }
    return true;
}

bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

uint32_t saturate(uint64_t v)
{
    return v > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(v);
}

// Names compare without regard to ASCII case, then by file name, so the
// order never depends on the order the folder lists its files in.
int compareNames(const Library::Entry& a, const Library::Entry& b)
{
    for (size_t i = 0;; ++i) {
        const char ca = lower(a.name[i]);
        const char cb = lower(b.name[i]);
        if (ca != cb) {
            return static_cast<unsigned char>(ca) < static_cast<unsigned char>(cb) ? -1 : 1;
        }
        if (ca == '\0') {
            break;
        }
    }
    return std::strcmp(a.file, b.file);
}

enum class Kind : uint8_t { Today, Upcoming, Past, Unscheduled };

Kind kind(const Library::Entry& e, Ymd today)
{
    if (e.date == 0) {
        return Kind::Unscheduled;
    }
    if (e.date == today) {
        return Kind::Today;
    }
    return e.date > today ? Kind::Upcoming : Kind::Past;
}

// Order within one kind, shared by both orders below.
bool before(const Library::Entry& a, const Library::Entry& b, Kind k)
{
    switch (k) {
    case Kind::Upcoming:    return a.date < b.date;
    case Kind::Past:        return a.date > b.date;
    case Kind::Unscheduled: return compareNames(a, b) < 0;
    default:                return std::strcmp(a.file, b.file) < 0;
    }
}

// The order to keep entries in when the list is full: today, the days to
// come, the others, then the days gone by.
int keepRank(Kind k)
{
    switch (k) {
    case Kind::Today:       return 0;
    case Kind::Upcoming:    return 1;
    case Kind::Unscheduled: return 2;
    default:                return 3;
    }
}

bool keepsOver(const Library::Entry& a, const Library::Entry& b, Ymd today)
{
    const Kind ka = kind(a, today);
    const Kind kb = kind(b, today);
    if (ka != kb) {
        return keepRank(ka) < keepRank(kb);
    }
    return before(a, b, ka);
}

}  // namespace

Ymd scheduleDate(const char* fileName)
{
    // "YYYY-MM-DD.fit"
    if (std::strlen(fileName) != 14 || !hasFitExtension(fileName, 14) || fileName[4] != '-'
        || fileName[7] != '-') {
        return 0;
    }
    uint32_t v[3] = {};
    const int    starts[3] = {0, 5, 8};
    const int    lens[3]   = {4, 2, 2};
    for (int f = 0; f < 3; ++f) {
        for (int i = 0; i < lens[f]; ++i) {
            const char c = fileName[starts[f] + i];
            if (!isDigit(c)) {
                return 0;
            }
            v[f] = v[f] * 10u + static_cast<uint32_t>(c - '0');
        }
    }
    const uint32_t year = v[0], month = v[1], day = v[2];
    static constexpr uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year == 0 || month < 1 || month > 12 || day < 1) {
        return 0;
    }
    const bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    const uint32_t days = kDays[month - 1] + (month == 2 && leap ? 1u : 0u);
    if (day > days) {
        return 0;
    }
    return year * 10000u + month * 100u + day;
}

void Library::scan(SDK::Interface::IFileSystem& fs, Ymd today, SDK::Fit::FitWorkoutReader& reader,
                   Program& scratch)
{
    mCount   = 0;
    mDropped = 0;
    mToday   = today;
    scanDir(fs, true, reader, scratch);
    scanDir(fs, false, reader, scratch);

    // Display order: today, the days to come, the days gone by, the others.
    const Ymd t = mToday;
    std::sort(mEntries, mEntries + mCount, [t](const Entry& a, const Entry& b) {
        const Kind ka = kind(a, t);
        const Kind kb = kind(b, t);
        if (ka != kb) {
            return static_cast<uint8_t>(ka) < static_cast<uint8_t>(kb);
        }
        return before(a, b, ka);
    });
}

void Library::scanDir(SDK::Interface::IFileSystem& fs, bool schedule,
                      SDK::Fit::FitWorkoutReader& reader, Program& scratch)
{
    const char* dirPath = schedule ? kScheduleDir : kWorkoutsDir;
    auto dir = fs.dir(dirPath);
    if (!dir || !dir->open()) {
        return;
    }
    char path[sizeof(kScheduleDir) + 1 + kFileBytes];
    while (dir->readNext(mItem)) {
        const size_t len = std::strlen(mItem.name);
        if (mItem.isDir || mItem.isHidden || len >= kFileBytes
            || !hasFitExtension(mItem.name, len)) {
            continue;
        }
        std::snprintf(path, sizeof(path), "%s/%s", dirPath, mItem.name);
        auto file = fs.file(path);
        if (!file || reader.read(*file, scratch) != SDK::Fit::FitWorkoutReader::Result::Ok) {
            continue;
        }
        // Running, generic, or no sport given.
        if (scratch.sport != static_cast<uint8_t>(SDK::Fit::Sport::Running)
            && scratch.sport != static_cast<uint8_t>(SDK::Fit::Sport::Generic)
            && scratch.sport != 0xFF) {
            continue;
        }

        Entry e;
        std::memcpy(e.file, mItem.name, len + 1);
        if (scratch.name[0] != '\0') {
            std::memcpy(e.name, scratch.name, sizeof(e.name));
        } else {
            const size_t n = std::min(len - 4, sizeof(e.name) - 1);  // without ".fit"
            std::memcpy(e.name, mItem.name, n);
            e.name[n] = '\0';
        }
        e.inSchedule = schedule;
        e.date       = schedule ? scheduleDate(mItem.name) : 0;
        e.degraded   = scratch.degraded;
        const Totals t = totals(scratch);
        e.timeMs     = saturate(t.timeMs);
        e.distanceCm = saturate(t.distanceCm);
        e.steps      = saturate(t.stepsRun);
        e.openSteps  = saturate(t.openSteps);
        offer(e);
    }
    dir->close();
}

void Library::offer(const Entry& e)
{
    if (mCount < kMaxEntries) {
        mEntries[mCount++] = e;
        return;
    }
    // Full: the new entry takes the place of the one least worth keeping,
    // if it is worth more.
    ++mDropped;
    size_t worst = 0;
    for (size_t i = 1; i < mCount; ++i) {
        if (keepsOver(mEntries[worst], mEntries[i], mToday)) {
            worst = i;
        }
    }
    if (keepsOver(e, mEntries[worst], mToday)) {
        mEntries[worst] = e;
    }
}

size_t Library::today() const
{
    if (mToday == 0) {
        return kNone;
    }
    for (size_t i = 0; i < mCount; ++i) {
        if (mEntries[i].date == mToday) {
            return i;
        }
    }
    return kNone;
}

bool Library::path(size_t i, char* buf, size_t cap) const
{
    if (i >= mCount) {
        return false;
    }
    const int n = std::snprintf(buf, cap, "%s/%s",
                                mEntries[i].inSchedule ? kScheduleDir : kWorkoutsDir,
                                mEntries[i].file);
    return n > 0 && static_cast<size_t>(n) < cap;
}

}  // namespace SDK::Workout
