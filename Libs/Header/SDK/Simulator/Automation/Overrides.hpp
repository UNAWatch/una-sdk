/**
 * @file Overrides.hpp
 * @brief Sensor values a simulator script sets in place of the simulated ones.
 *
 * Simulator only. The simulated sensors (SDK/Simulator/Components/Simulator)
 * produce their values from the app's ConfigurationSimulator.hpp. A script
 * (Script.hpp) can replace them while it runs: a fixed heart rate, battery
 * level or pressure, a GPS fix forced on or off, or a GPS track replayed from a
 * CSV file. Each simulated sensor asks here first and falls back to its own
 * value when nothing is set. Thread-safe: the script runs on the GUI thread,
 * the sensors on their own.
 *
 * Replay file: one fix per line, comma separated,
 *
 *     t_s,lat,lon,alt_m,speed_mps
 *     0,51.50740,-0.12780,11.0,2.8
 *     1,51.50742,-0.12776,11.1,2.9
 *
 * t_s is seconds from the start of the replay, increasing; lat and lon are
 * degrees, speed_mps is 0 to 300. A first line that is
 * not numbers is taken as a header. Until the first line's time the fix is that
 * line's; between two lines it is interpolated; after the last one it stays at
 * the last position with speed 0.
 *
 * Starting or stopping a replay moves the position straight to the other
 * track, which can be far away; an app that sums distance from positions
 * counts that jump. Start a replay at time 0, before the app has a fix.
 */

#ifndef SDK_SIMULATOR_AUTOMATION_OVERRIDES_HPP
#define SDK_SIMULATOR_AUTOMATION_OVERRIDES_HPP

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace SDK::Simulator::Automation
{

class Overrides
{
public:
    /// GPS fix as the script set it.
    enum class GpsFix { Auto, On, Off };

    struct TrackPoint {
        double t     = 0.0;   ///< s from the start of the replay (from replayFix(): now)
        double lat   = 0.0;
        double lon   = 0.0;
        float  alt   = 0.0f;  ///< m
        float  speed = 0.0f;  ///< m/s
    };

    /// The fastest speed a replay file may give, m/s.
    static constexpr float kMaxReplaySpeed = 300.0f;

    static Overrides& get()
    {
        static Overrides sOverrides;
        return sOverrides;
    }

    // --- heart rate, bpm (< 0: simulated) ---
    void setHeartRate(int bpm)  { std::lock_guard<std::mutex> l(mMutex); mHeartRate = bpm; }
    bool heartRate(int& bpm) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        bpm = mHeartRate;
        return mHeartRate >= 0;
    }

    // --- battery, % (< 0: simulated) ---
    void setBattery(float percent) { std::lock_guard<std::mutex> l(mMutex); mBattery = percent; }
    bool battery(float& percent) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        percent = mBattery;
        return mBattery >= 0.0f;
    }

    // --- pressure, hPa (<= 0: simulated) ---
    void setPressure(float hpa) { std::lock_guard<std::mutex> l(mMutex); mPressure = hpa; }
    bool pressure(float& hpa) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        hpa = mPressure;
        return mPressure > 0.0f;
    }

    // --- GPS ---
    void setGpsFix(GpsFix fix) { std::lock_guard<std::mutex> l(mMutex); mGpsFix = fix; }
    GpsFix gpsFix() const      { std::lock_guard<std::mutex> l(mMutex); return mGpsFix; }

    // --- Wrist motion ---
    /// Set by the simulator's sensor layer, which not every simulator links
    /// (an app without sensors leaves it out).
    void setWristHandler(std::function<void()> handler)
    {
        std::lock_guard<std::mutex> l(mMutex);
        mWristHandler = std::move(handler);
    }

    /// A wrist-raise motion now. False if this simulator has no sensor layer.
    bool wristMotion()
    {
        std::function<void()> handler;
        {
            std::lock_guard<std::mutex> l(mMutex);
            handler = mWristHandler;
        }
        if (!handler) {
            return false;
        }
        handler();
        return true;
    }

    /// Load a track and start replaying it now. False (and nothing changes) if
    /// the file can't be read or holds no fixes; error says why.
    bool startReplay(const std::string& path, std::string& error)
    {
        std::vector<TrackPoint> track;
        if (!loadTrack(path, track, error)) {
            return false;
        }
        std::lock_guard<std::mutex> l(mMutex);
        mTrack       = std::move(track);
        mReplayStart = std::chrono::steady_clock::now();
        ++mReplayId;
        return true;
    }

    void stopReplay() { std::lock_guard<std::mutex> l(mMutex); mTrack.clear(); }

    /// The replayed fix for now; false when no replay is running.
    bool replayFix(TrackPoint& fix) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        if (mTrack.empty()) {
            return false;
        }
        fix = fixAt(replayTime());
        return true;
    }

    /// Where a sensor that counts distance has got to in a replay.
    struct ReplayCursor {
        uint64_t replay = 0;     ///< which replay (0: none yet)
        double   t      = 0.0;   ///< s into it at the previous call
    };

    /**
     * @brief The replayed fix for now, and the distance along the track since
     *        the cursor's previous call (from the start, for a new replay).
     * @return false when no replay is running.
     */
    bool replayAdvance(ReplayCursor& cursor, TrackPoint& fix, double& metres) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        if (mTrack.empty()) {
            return false;
        }
        const double t    = replayTime();
        const double from = cursor.replay == mReplayId ? cursor.t : 0.0;
        fix    = fixAt(t);
        metres = distanceBetween(from, t);
        cursor = ReplayCursor { mReplayId, t };
        return true;
    }

    /// Move the cursor to now without counting the distance meanwhile (a sensor
    /// that has lost its fix).
    void replaySkip(ReplayCursor& cursor) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        if (!mTrack.empty()) {
            cursor = ReplayCursor { mReplayId, replayTime() };
        }
    }

    /// Distance along the running replay between two times (s into it), from
    /// its speeds; 0 when no replay is running.
    double trackDistance(double from, double to) const
    {
        std::lock_guard<std::mutex> l(mMutex);
        return mTrack.empty() ? 0.0 : distanceBetween(from, to);
    }

private:
    /// s since the running replay started. Call with mMutex held.
    double replayTime() const
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - mReplayStart).count();
    }

    /// The fix at t s into the replay. mTrack not empty; call with mMutex held.
    TrackPoint fixAt(double t) const
    {
        TrackPoint fix;
        if (t >= mTrack.back().t) {
            fix       = mTrack.back();
            fix.t     = t;
            fix.speed = 0.0f;
            return fix;
        }
        if (t <= mTrack.front().t) {
            // Before the first fix (a track need not start at 0): hold it.
            fix   = mTrack.front();
            fix.t = t;
            return fix;
        }
        size_t i = 1;
        while (i < mTrack.size() && mTrack[i].t <= t) {
            ++i;
        }
        const TrackPoint& a = mTrack[i - 1];
        const TrackPoint& b = mTrack[i];
        const double span = b.t - a.t;
        const double k = span > 0.0 ? (t - a.t) / span : 0.0;
        fix.t     = t;
        fix.lat   = a.lat + (b.lat - a.lat) * k;
        fix.lon   = a.lon + (b.lon - a.lon) * k;
        fix.alt   = static_cast<float>(a.alt + (b.alt - a.alt) * k);
        fix.speed = static_cast<float>(a.speed + (b.speed - a.speed) * k);
        return fix;
    }

    /// Distance between two times, integrating the speed (linear between
    /// fixes). Nothing moves before the first fix or after the last.
    /// mTrack not empty; call with mMutex held.
    double distanceBetween(double from, double to) const
    {
        // (std::max): a Windows.h max macro may be defined.
        from = (std::max)(from, mTrack.front().t);
        to   = (std::min)(to, mTrack.back().t);
        double metres = 0.0;
        for (size_t i = 1; i < mTrack.size() && from < to; ++i) {
            const TrackPoint& a = mTrack[i - 1];
            const TrackPoint& b = mTrack[i];
            const double x = (std::max)(from, a.t);
            const double y = (std::min)(to, b.t);
            if (y <= x) {
                continue;
            }
            const double span  = b.t - a.t;
            const auto   speed = [&](double t) { return a.speed + (b.speed - a.speed) * (t - a.t) / span; };
            metres += (speed(x) + speed(y)) / 2.0 * (y - x);
        }
        return metres;
    }

public:

    /// Parse a replay file (see the file comment).
    static bool loadTrack(const std::string& path, std::vector<TrackPoint>& track, std::string& error)
    {
        std::ifstream in(path);
        if (!in) {
            error = "cannot open " + path;
            return false;
        }
        std::string line;
        int number = 0;
        while (std::getline(in, line)) {
            ++number;
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty() || line[0] == '#') {
                continue;
            }
            TrackPoint p;
            // " %n": where the row ends after any trailing whitespace; anything
            // left there (a unit, a sixth field) makes the row invalid.
            int end = -1;
            const int n = std::sscanf(line.c_str(), "%lf,%lf,%lf,%f,%f %n", &p.t, &p.lat, &p.lon, &p.alt, &p.speed, &end);
            if (n != 5 || end < 0 || line[static_cast<size_t>(end)] != '\0') {
                if (n == 0 && track.empty() && number == 1) {
                    continue; // a header: the first line, not starting with a number
                }
                error = path + ":" + std::to_string(number) + ": expected t_s,lat,lon,alt_m,speed_mps";
                return false;
            }
            if (!std::isfinite(p.t) || !std::isfinite(p.lat) || !std::isfinite(p.lon) ||
                !std::isfinite(p.alt) || !std::isfinite(p.speed)) {
                error = path + ":" + std::to_string(number) + ": values must be finite numbers";
                return false;
            }
            if (p.t < 0.0 || p.lat < -90.0 || p.lat > 90.0 || p.lon < -180.0 || p.lon > 180.0 ||
                p.speed < 0.0f || p.speed > kMaxReplaySpeed) {
                error = path + ":" + std::to_string(number) +
                        ": out of range (t_s >= 0, lat -90..90, lon -180..180, speed_mps 0..300)";
                return false;
            }
            if (!track.empty() && p.t <= track.back().t) {
                error = path + ":" + std::to_string(number) + ": t_s must increase";
                return false;
            }
            track.push_back(p);
        }
        if (track.empty()) {
            error = path + ": no fixes";
            return false;
        }
        return true;
    }

private:
    Overrides() = default;

    mutable std::mutex mMutex;
    int     mHeartRate = -1;
    float   mBattery   = -1.0f;
    float   mPressure  = 0.0f;
    GpsFix  mGpsFix    = GpsFix::Auto;
    std::vector<TrackPoint>               mTrack;
    std::chrono::steady_clock::time_point mReplayStart;
    uint64_t                              mReplayId = 0;   ///< counts startReplay() calls
    std::function<void()>                 mWristHandler;
};

} // namespace SDK::Simulator::Automation

#endif // SDK_SIMULATOR_AUTOMATION_OVERRIDES_HPP
