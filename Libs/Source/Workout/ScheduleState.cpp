/**
 ******************************************************************************
 * @file    ScheduleState.cpp
 * @brief   Whether the scheduled workout last started was completed.
 ******************************************************************************
 */

#include "SDK/Workout/ScheduleState.hpp"

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string_view>

namespace SDK::Workout {

namespace {

// The record is about 70 bytes; anything much larger is not one.
constexpr size_t kMaxFileBytes = 256;

// "YYYY-MM-DD" from a date, or 0 if @p s is not one.
Ymd parseDate(std::string_view s)
{
    if (s.size() != 10) {
        return 0;
    }
    char name[16];
    std::memcpy(name, s.data(), 10);
    std::memcpy(name + 10, ".fit", 5);
    return scheduleDate(name);
}

}  // namespace

void ScheduleState::load()
{
    mDate = 0;
    mFile[0] = '\0';
    mCompleted = false;

    auto file = mFs.file(mPath);
    if (!file || !file->open(false, false)) {
        return;
    }
    char   buf[kMaxFileBytes];
    size_t n = 0;
    const bool read = file->size() < sizeof(buf) && file->read(buf, sizeof(buf), n);
    file->close();
    if (!read) {
        return;
    }

    SDK::JsonStreamReader json(buf, n);
    if (!json.validate()) {
        return;
    }
    std::string_view date;
    std::string_view name;
    bool             completed = false;
    if (!json.get("date", date) || !json.get("file", name) || !json.get("completed", completed)
        || name.empty() || name.size() >= sizeof(mFile)) {
        return;
    }
    const Ymd d = parseDate(date);
    if (d == 0) {
        return;
    }
    mDate = d;
    std::memcpy(mFile, name.data(), name.size());
    mFile[name.size()] = '\0';
    mCompleted = completed;
}

bool ScheduleState::isRecord(Ymd date, const char* file) const
{
    return date != 0 && date == mDate && std::strcmp(file, mFile) == 0;
}

bool ScheduleState::started(Ymd date, const char* file)
{
    const size_t len = std::strlen(file);
    if (date == 0 || len >= sizeof(mFile)) {
        return false;
    }
    if (isRecord(date, file)) {
        return true;
    }
    mDate = date;
    std::memcpy(mFile, file, len + 1);
    mCompleted = false;
    return save();
}

bool ScheduleState::completed()
{
    if (mDate == 0) {
        return false;
    }
    if (mCompleted) {
        return true;
    }
    mCompleted = true;
    return save();
}

bool ScheduleState::isCompleted(Ymd date, const char* file) const
{
    return isRecord(date, file) && mCompleted;
}

bool ScheduleState::save()
{
    auto file = mFs.file(mPath);
    if (!file || !file->open(true, true)) {
        return false;
    }
    char date[16];
    std::snprintf(date, sizeof(date), "%04u-%02u-%02u", static_cast<unsigned>(mDate / 10000u),
                  static_cast<unsigned>(mDate / 100u % 100u), static_cast<unsigned>(mDate % 100u));
    SDK::JsonStreamWriter json(file.get());
    json.startMap();
    json.add("date", date);
    json.add("file", mFile);
    json.add("completed", mCompleted);
    json.endMap();
    const bool ok = !json.isError();
    return file->close() && ok;
}

}  // namespace SDK::Workout
