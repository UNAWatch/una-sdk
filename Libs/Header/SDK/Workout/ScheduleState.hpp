/**
 ******************************************************************************
 * @file    ScheduleState.hpp
 * @brief   Whether the scheduled workout last started was completed.
 *
 * A small JSON file in the app's own folder records the scheduled workout
 * most recently started, by its date and file, and whether it was
 * completed:
 *
 *     {"date":"2026-10-09","file":"2026-10-09.fit","completed":false}
 *
 * Starting a different scheduled workout replaces the record. The record
 * stays with the workout it was started for, so a run that crosses midnight
 * still completes the day it began on. Completion is never undone.
 *
 * A new record is written in full to the same path plus ".tmp", which then
 * replaces the file. A write that fails leaves the record before in place.
 * Once the old file is removed the ".tmp" file is the record: if renaming it
 * fails or is cut short, load() and the next save put it in place first.
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_SCHEDULE_STATE_HPP
#define SDK_WORKOUT_SCHEDULE_STATE_HPP

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"

#include <cstddef>

namespace SDK::Workout {

class ScheduleState {
public:
    static constexpr char kDefaultPath[] = "workouts_state.json";

    explicit ScheduleState(SDK::Interface::IFileSystem& fs, const char* path = kDefaultPath)
        : mFs(fs), mPath(path) {}

    /// Read the record. A missing or unreadable file reads as no record.
    void load();

    /// The scheduled workout in @p file, planned for @p date, has started.
    /// Saves a new record unless this workout already has one. If the save
    /// fails, the record before is kept and false returned.
    bool started(Ymd date, const char* file);

    /// The workout last started has been completed, and its activity saved.
    /// If the save fails, it stays not completed and false is returned, so
    /// a later call tries again.
    bool completed();

    /// True if the workout in @p file, planned for @p date, was completed.
    bool isCompleted(Ymd date, const char* file) const;

private:
    bool save();
    bool write(const char* path);
    bool finishReplacement(const char* tmp);
    bool loadFrom(const char* path);
    bool tempPath(char* buf, size_t cap) const;
    bool isRecord(Ymd date, const char* file) const;

    SDK::Interface::IFileSystem& mFs;
    const char*                  mPath;
    Ymd                          mDate = 0;
    char                         mFile[Library::kFileBytes] = {};
    bool                         mCompleted = false;
};

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_SCHEDULE_STATE_HPP
