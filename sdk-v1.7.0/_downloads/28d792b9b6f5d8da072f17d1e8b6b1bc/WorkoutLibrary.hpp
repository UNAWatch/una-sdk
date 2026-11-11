/**
 ******************************************************************************
 * @file    WorkoutLibrary.hpp
 * @brief   The structured workouts on the watch, as a list to choose from.
 *
 * Workouts are FIT workout files in kWorkoutsDir, a folder shared by all apps
 * and read-only to them. Those in kScheduleDir are named for the day they are
 * planned for, "YYYY-MM-DD.fit"; a schedule file with any other name is
 * listed with the rest.
 *
 * Library::scan() reads every .fit file in both folders, whole, into one
 * reused Program, and keeps a short entry for each that is a valid running
 * (or generic) workout. Files that fail to read are left out.
 *
 * The list is in display order:
 *   - scheduled workouts: today's, then the days to come, soonest first, then
 *     the days gone by, most recent first;
 *   - the others, by name.
 * It holds at most kMaxEntries. Beyond that, past scheduled workouts are
 * dropped first, oldest first, then the others from the end of the name
 * order, then the furthest days to come.
 ******************************************************************************
 */

#ifndef SDK_WORKOUT_LIBRARY_HPP
#define SDK_WORKOUT_LIBRARY_HPP

#include "SDK/Fit/FitWorkoutReader.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstddef>
#include <cstdint>

namespace SDK::Workout {

/// The shared workouts folder, at the root of the apps' volume.
constexpr char kWorkoutsDir[] = ":/Workouts";
/// Workouts planned for a day, named "YYYY-MM-DD.fit".
constexpr char kScheduleDir[] = ":/Workouts/Schedule";

/// A calendar date as the number yyyymmdd (20261009 for 9 October 2026); 0
/// for no date.
using Ymd = uint32_t;

/// The date in a schedule file name, "YYYY-MM-DD.fit" (any case of
/// extension), or 0 if @p fileName is not one or names no real day.
Ymd scheduleDate(const char* fileName);

class Library {
public:
    static constexpr size_t kMaxEntries = 64;
    /// File name bytes, NUL included. Files with longer names are not listed.
    static constexpr size_t kFileBytes = 64;
    static constexpr size_t kNone = SIZE_MAX;

    struct Entry {
        char     file[kFileBytes] = {};  ///< File name within its folder.
        char     name[kNameBytes] = {};  ///< Workout name, or the file name without ".fit".
        Ymd      date = 0;               ///< The day it is planned for; 0 if not scheduled.
        bool     inSchedule = false;     ///< In kScheduleDir (date may still be 0).
        bool     degraded = false;       ///< Some step runs in a simplified form.
        // Totals with repeats expanded, saturating at UINT32_MAX.
        uint32_t timeMs     = 0;         ///< Sum of the time steps.
        uint32_t distanceCm = 0;         ///< Sum of the distance steps.
        uint32_t steps      = 0;         ///< Step instances run.
        uint32_t openSteps  = 0;         ///< Of those, open steps (+ unknown time and distance).
    };

    /// Rebuild the list from @p fs. @p today is the watch's local date.
    /// @p reader and @p scratch are used to read each file; @p scratch holds
    /// the last file read afterwards.
    void scan(SDK::Interface::IFileSystem& fs, Ymd today, SDK::Fit::FitWorkoutReader& reader,
              Program& scratch);

    size_t       size() const { return mCount; }
    const Entry& entry(size_t i) const { return mEntries[i]; }

    /// Index of the workout planned for today, or kNone.
    size_t today() const;

    /// The path of entry @p i, for reading it again. False if it does not
    /// fit in @p cap bytes.
    bool path(size_t i, char* buf, size_t cap) const;

    /// Valid workouts left out because the list was full, at the last scan.
    size_t dropped() const { return mDropped; }

private:
    void scanDir(SDK::Interface::IFileSystem& fs, bool schedule, SDK::Fit::FitWorkoutReader& reader,
                 Program& scratch);
    void offer(const Entry& e);

    Entry  mEntries[kMaxEntries];
    size_t mCount   = 0;
    size_t mDropped = 0;
    Ymd    mToday   = 0;
    SDK::Interface::IFileSystem::ObjectInfo mItem{};  ///< 256 B: kept off the stack.
};

}  // namespace SDK::Workout

#endif  // SDK_WORKOUT_LIBRARY_HPP
