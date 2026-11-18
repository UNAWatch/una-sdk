/**
 * @file Script.hpp
 * @brief Timeline scripts for simulator runs: buttons, sensor values, screenshots.
 *
 * Simulator only. A script is a text file, one step per line:
 *
 *     # comments start with '#'
 *     0       gps replay course.csv
 *     0       gps fix off
 *     2000    gps fix on
 *     +1000   click R1            # +N: N ms after the previous step
 *     5s      hold R1 3500        # times may also be given in seconds
 *     +500    screenshot running.png
 *     +0      hr 152
 *     30s     exit
 *
 * Times are watch time in ms from the start of the GUI (UNA_SIM_SLOW
 * stretches them with the frames), may not go backwards and end at
 * 4294967295 ms. With --duration-ms, nothing after it runs.
 *
 * Steps:
 *
 *     click B            press and release button B (L1, L2, R1, R2), 100 ms apart
 *     hold B MS          press B and release it MS (whole) ms later
 *     press B            press B and keep it down
 *     release B          release B
 *     screenshot FILE    save the display as PNG in the --out directory (no absolute path,
 *                        no .., not sim.log)
 *     hr BPM | hr auto   fixed heart rate (whole, 0-255), or back to the simulated one
 *     battery PCT | battery auto     (0-100)
 *     pressure HPA | pressure auto   (a positive number)
 *     gps fix on|off|auto      force the fix; "on" also skips the search delay
 *     gps replay FILE          replay a CSV track (see Overrides.hpp), relative to the script
 *     gps auto                 stop the replay, back to the simulated track
 *     wrist              a wrist-raise motion
 *     log TEXT           write TEXT to the console and sim.log, to mark a point in the log
 *     exit               end the run
 *
 * Buttons reach the app as they do on the watch: a press, then on release a
 * click (if the button was down less than 500 ms) and the release. A click,
 * hold or press of a button already down does nothing.
 *
 * A host (the TouchGFX HAL, the LVGL host, the glance host) owns a Runner and
 * calls tick() once per frame on its GUI thread; the Runner calls the host back
 * through ScriptHost for everything that needs the host's own API.
 */

#ifndef SDK_SIMULATOR_AUTOMATION_SCRIPT_HPP
#define SDK_SIMULATOR_AUTOMATION_SCRIPT_HPP

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "SDK/Simulator/Automation/Options.hpp"
#include "SDK/Simulator/Automation/Overrides.hpp"

namespace SDK::Simulator::Automation
{

enum class Button { L1 = 0, L2 = 1, R1 = 2, R2 = 3 };

inline const char* buttonName(Button b)
{
    static const char* names[] = { "L1", "L2", "R1", "R2" };
    return names[static_cast<int>(b)];
}

/// What a Runner needs from the simulator it drives.
class ScriptHost
{
public:
    virtual ~ScriptHost() = default;

    /// False for a host with no buttons (a glance): button steps are then an error.
    virtual bool hasButtons() const { return true; }

    virtual void buttonDown(Button b) = 0;

    /// @param click True when the press was short enough to count as a click.
    virtual void buttonUp(Button b, bool click) = 0;

    /// Save the display as it is now. False on failure.
    virtual bool saveDisplay(const std::string& path) = 0;

    virtual void wristMotion() = 0;

    /// End the run: the host shuts its loop down normally.
    virtual void endRun() = 0;
};

class Runner
{
public:
    /// The kernel reports a CLICK for a press shorter than this (HWButtons.cpp).
    static constexpr uint32_t kClickMaxMs = 500;
    static constexpr uint32_t kClickHoldMs = 100;
    /// The latest time a script can name, steps and releases alike.
    static constexpr uint32_t kMaxTimeMs = UINT32_MAX;

    /**
     * @param host        The simulator.
     * @param ownArgs     True if the host has command-line options of its own
     *                    (then unknown arguments are not reported here).
     */
    explicit Runner(ScriptHost& host, bool ownArgs = false)
        : mHost(host)
    {
        Options& o = Options::get();
        if (!ownArgs) {
            for (const std::string& a : o.unknownArgs()) {
                o.report("warning: unknown argument '%s' (options: --script FILE --headless "
                         "--duration-ms N --out DIR --fs-root DIR)", a.c_str());
            }
        }
        if (o.scripted()) {
            loadScript(o.scriptPath());
        }
        if (o.headless() && o.durationMs() == 0 && !mHasExit) {
            o.abortRun("--headless needs --duration-ms N or an 'exit' step, or it would never end");
        }
        if (o.automated()) {
            o.report("automated run: %s%s, output in %s", o.headless() ? "headless" : "windowed",
                     o.scripted() ? (", script " + o.scriptPath()).c_str() : "",
                     o.outPath("").c_str());
        }
    }

    /// Call once per frame on the GUI thread.
    void tick()
    {
        const auto wallNow = std::chrono::steady_clock::now();
        if (!mStarted) {
            mStarted = true;
            mStart   = wallNow;
        }
        tickAt(Options::toWatchMs(wallNow - mStart));
    }

    /// tick() at a given watch time (ms from the start); times must not go back.
    void tickAt(uint32_t now)
    {
        if (mStopped) {
            return;
        }
        // A late frame runs nothing scripted after the end of --duration-ms.
        const uint32_t duration = Options::get().durationMs();
        if (duration != 0 && now > duration) {
            now = duration;
        }

        // Everything due by now, in the order of its own time: the steps and
        // the releases that click/hold scheduled. A release goes before a step
        // with the same time. Each runs at its scripted time, not the frame's,
        // so a hold's length (click or not) doesn't depend on the frame rate.
        for (;;) {
            const bool release = !mReleases.empty() && mReleases.front().at <= now;
            const bool step    = mNext < mSteps.size() && mSteps[mNext].at <= now;
            if (release && (!step || mReleases.front().at <= mSteps[mNext].at)) {
                const Release r = mReleases.front();
                mReleases.erase(mReleases.begin());
                // Only the press it was scheduled for: the button may have been
                // released and pressed again since.
                if (mPressId[static_cast<int>(r.button)] == r.pressId) {
                    up(r.button, r.at);
                }
            } else if (step) {
                run(mSteps[mNext], mSteps[mNext].at);
                ++mNext;
                if (mStopped) {
                    return;
                }
            } else {
                break;
            }
        }

        if (duration != 0 && now >= duration) {
            Options::get().report("t=%u: --duration-ms reached", now);
            stopRun();
        }
    }

    bool stopped() const { return mStopped; }

    /// Read and check a script; a bad one ends the process (Options::abortRun).
    void loadScript(const std::string& path)
    {
        load(path);
    }

private:
    struct Step {
        uint32_t                 at = 0;
        int                      line = 0;
        std::string              verb;
        std::vector<std::string> args;
        std::string              text;   ///< the step as written, for the log
    };
    struct Release {
        uint32_t at;
        Button   button;
        uint32_t pressId;   ///< which press of the button it ends
    };

    void load(const std::string& path)
    {
        Options& o = Options::get();
        std::ifstream in(path);
        if (!in) {
            o.abortRun("cannot open script %s", path.c_str());
        }
        mScriptDir = std::filesystem::path(path).parent_path();

        std::string line;
        int         number = 0;
        uint32_t    last   = 0;
        while (std::getline(in, line)) {
            ++number;
            const size_t hash = line.find('#');
            if (hash != std::string::npos) {
                line.erase(hash);
            }
            std::istringstream words(line);
            std::string time;
            if (!(words >> time)) {
                continue;
            }
            Step s;
            s.line = number;
            s.at   = parseTime(time, last, path, number);
            if (s.at < last) {
                o.abortRun("%s:%d: time %u is before the previous step (%u)", path.c_str(), number, s.at, last);
            }
            last = s.at;
            if (!(words >> s.verb)) {
                o.abortRun("%s:%d: a time with no step", path.c_str(), number);
            }
            std::string w;
            while (words >> w) {
                s.args.push_back(w);
            }
            s.text = s.verb;
            for (const std::string& a : s.args) {
                s.text += " " + a;
            }
            validate(s, path);
            if (s.verb == "exit") {
                mHasExit = true;
            }
            mSteps.push_back(std::move(s));
        }
        if (in.bad()) {
            // Not the end of the file: don't run what was read so far.
            o.abortRun("%s: read error after line %d", path.c_str(), number);
        }
    }

    static uint32_t parseTime(const std::string& word, uint32_t previous, const std::string& path, int line)
    {
        std::string t = word;
        bool relative = false;
        if (!t.empty() && t[0] == '+') {
            relative = true;
            t.erase(0, 1);
        }
        double scale = 1.0;
        if (!t.empty() && t.back() == 's') {
            scale = 1000.0;
            t.pop_back();
        }
        char* end = nullptr;
        const double v = t.empty() ? -1.0 : std::strtod(t.c_str(), &end);
        if (t.empty() || *end != '\0' || v < 0.0 || !std::isfinite(v)) {
            Options::get().abortRun("%s:%d: '%s' is not a time (N ms, Ns, +N or +Ns)", path.c_str(), line, word.c_str());
        }
        // Times are uint32_t ms of watch time; one that doesn't fit is an error.
        const double ms = std::round(v * scale);
        const double at = relative ? previous + ms : ms;
        if (at > static_cast<double>(kMaxTimeMs)) {
            Options::get().abortRun("%s:%d: '%s' is later than the latest time, %u ms", path.c_str(), line,
                                    word.c_str(), kMaxTimeMs);
        }
        return static_cast<uint32_t>(at);
    }

    static bool parseButton(const std::string& word, Button& b)
    {
        std::string u = word;
        std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        if (u == "L1") { b = Button::L1; return true; }
        if (u == "L2") { b = Button::L2; return true; }
        if (u == "R1") { b = Button::R1; return true; }
        if (u == "R2") { b = Button::R2; return true; }
        return false;
    }

    static bool parseNumber(const std::string& word, double& v)
    {
        char* end = nullptr;
        v = std::strtod(word.c_str(), &end);
        return !word.empty() && *end == '\0' && std::isfinite(v);
    }

    void validate(const Step& s, const std::string& path) const
    {
        Options& o = Options::get();
        auto bad = [&](const char* usage) {
            o.abortRun("%s:%d: '%s': usage: %s", path.c_str(), s.line, s.text.c_str(), usage);
        };
        const size_t n = s.args.size();
        Button b;
        double v = 0.0;
        if (s.verb == "click" || s.verb == "press" || s.verb == "release" || s.verb == "hold") {
            if (!mHost.hasButtons()) {
                o.abortRun("%s:%d: this simulator has no buttons", path.c_str(), s.line);
            }
            if (s.verb == "hold") {
                if (n != 2 || !parseButton(s.args[0], b) || !parseNumber(s.args[1], v) || v < 1 ||
                    v != std::floor(v)) {
                    bad("hold L1|L2|R1|R2 MS (whole ms)");
                }
                if (v > static_cast<double>(kMaxTimeMs - s.at)) {
                    bad("hold L1|L2|R1|R2 MS, released by the latest time");
                }
            } else if (n != 1 || !parseButton(s.args[0], b)) {
                bad("click|press|release L1|L2|R1|R2");
            } else if (s.verb == "click" && s.at > kMaxTimeMs - kClickHoldMs) {
                bad("click L1|L2|R1|R2, released by the latest time");
            }
        } else if (s.verb == "screenshot") {
            // A file name inside --out: no absolute path, no "..".
            const std::filesystem::path p = n == 1 ? std::filesystem::path(s.args[0]) : std::filesystem::path();
            bool inside = n == 1 && !p.has_root_name() && !p.has_root_directory();
            for (const auto& part : p) {
                inside = inside && part != "..";
            }
            if (!inside) {
                bad("screenshot NAME (a file name inside --out)");
            }
            // sim.log is the run's log, open while it runs. Windows ignores
            // trailing dots and spaces and reads ':' as a stream, so "sim.log."
            // and "sim.log::$DATA" are it too: no ':' at all, and trailing dots
            // and spaces dropped before comparing.
            std::string normal = screenshotName(s.args[0]);
            std::transform(normal.begin(), normal.end(), normal.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            while (!normal.empty() && (normal.back() == '.' || normal.back() == ' ')) {
                normal.pop_back();
            }
            if (s.args[0].find(':') != std::string::npos) {
                bad("screenshot NAME (no ':')");
            }
            if (normal == "sim.log") {
                bad("screenshot NAME (any name but sim.log, the run's log)");
            }
        } else if (s.verb == "hr") {
            if (n != 1 || (s.args[0] != "auto" && (!parseNumber(s.args[0], v) || v < 0 || v > 255 ||
                                                   v != std::floor(v)))) {
                bad("hr BPM (a whole number, 0-255) | hr auto");
            }
        } else if (s.verb == "battery") {
            if (n != 1 || (s.args[0] != "auto" && (!parseNumber(s.args[0], v) || v < 0 || v > 100))) {
                bad("battery PCT (0-100) | battery auto");
            }
        } else if (s.verb == "pressure") {
            // Checked as the float the sensor gets: finite, and not rounded to 0
            // (which would mean "simulated").
            if (n != 1 || (s.args[0] != "auto" && (!parseNumber(s.args[0], v) || !(v > 0.0) ||
                                                   v > static_cast<double>(FLT_MAX) ||
                                                   !(static_cast<float>(v) > 0.0f)))) {
                bad("pressure HPA (a positive number) | pressure auto");
            }
        } else if (s.verb == "gps") {
            const bool ok = (n == 1 && s.args[0] == "auto") ||
                            (n == 2 && s.args[0] == "fix" && (s.args[1] == "on" || s.args[1] == "off" || s.args[1] == "auto")) ||
                            (n == 2 && s.args[0] == "replay");
            if (!ok) {
                bad("gps fix on|off|auto, gps replay FILE.csv, gps auto");
            }
            if (n == 2 && s.args[0] == "replay") {
                // Read it now, so a bad file fails before the run starts.
                std::vector<Overrides::TrackPoint> track;
                std::string error;
                if (!Overrides::loadTrack(resolve(s.args[1]), track, error)) {
                    o.abortRun("%s:%d: %s", path.c_str(), s.line, error.c_str());
                }
            }
        } else if (s.verb == "wrist" || s.verb == "exit") {
            if (n != 0) {
                bad("wrist | exit (no arguments)");
            }
        } else if (s.verb == "log") {
            // free text
        } else {
            o.abortRun("%s:%d: unknown step '%s' (click, hold, press, release, screenshot, hr, "
                       "battery, pressure, gps, wrist, log, exit)", path.c_str(), s.line, s.verb.c_str());
        }
    }

    std::string resolve(const std::string& file) const
    {
        const std::filesystem::path p(file);
        return p.is_absolute() ? file : (mScriptDir / p).string();
    }

    void run(const Step& s, uint32_t now)
    {
        Options& o = Options::get();
        o.report("t=%u line %d: %s", now, s.line, s.text.c_str());
        Overrides& ov = Overrides::get();
        Button b;
        double v = 0.0;

        // A click or hold on a button already down does nothing: its release
        // would end the press already under way.
        if (s.verb == "click") {
            parseButton(s.args[0], b);
            if (down(b, now)) {
                schedule(b, now + kClickHoldMs);
            }
        } else if (s.verb == "hold") {
            parseButton(s.args[0], b);
            parseNumber(s.args[1], v);
            if (down(b, now)) {
                schedule(b, now + static_cast<uint32_t>(v));
            }
        } else if (s.verb == "press") {
            parseButton(s.args[0], b);
            down(b, now);
        } else if (s.verb == "release") {
            parseButton(s.args[0], b);
            up(b, now);
        } else if (s.verb == "screenshot") {
            const std::string path = o.outPath(screenshotName(s.args[0]));
            if (mHost.saveDisplay(path)) {
                o.report("screenshot %s", path.c_str());
            } else {
                o.fail("line %d: could not save screenshot %s", s.line, path.c_str());
            }
        } else if (s.verb == "hr" || s.verb == "battery" || s.verb == "pressure") {
            // The value validate() accepted.
            const bool automatic = s.args[0] == "auto";
            if (!automatic) {
                parseNumber(s.args[0], v);
            }
            if (s.verb == "hr") {
                ov.setHeartRate(automatic ? -1 : static_cast<int>(v));
            } else if (s.verb == "battery") {
                ov.setBattery(automatic ? -1.0f : static_cast<float>(v));
            } else {
                ov.setPressure(automatic ? 0.0f : static_cast<float>(v));
            }
        } else if (s.verb == "gps") {
            if (s.args[0] == "auto") {
                ov.stopReplay();
                ov.setGpsFix(Overrides::GpsFix::Auto);
            } else if (s.args[0] == "fix") {
                ov.setGpsFix(s.args[1] == "on"  ? Overrides::GpsFix::On
                           : s.args[1] == "off" ? Overrides::GpsFix::Off
                                                : Overrides::GpsFix::Auto);
            } else {
                std::string error;
                if (!ov.startReplay(resolve(s.args[1]), error)) {
                    o.fail("line %d: %s", s.line, error.c_str());
                }
            }
        } else if (s.verb == "wrist") {
            mHost.wristMotion();
        } else if (s.verb == "exit") {
            stopRun();
        }
        // "log" is the report above.
    }

    /// The file a screenshot step writes, in --out: .png added if the name has
    /// no extension, and lexically normalised ("./a.png" is "a.png").
    static std::string screenshotName(const std::string& arg)
    {
        std::filesystem::path p = std::filesystem::path(arg).lexically_normal();
        if (p.extension().empty()) {
            p += ".png";
        }
        return p.generic_string();
    }

    /// Press a button; false (and nothing happens) if it is already down.
    bool down(Button b, uint32_t now)
    {
        const int i = static_cast<int>(b);
        if (mDown[i]) {
            return false;
        }
        mDown[i]   = true;
        mDownAt[i] = now;
        ++mPressId[i];
        mHost.buttonDown(b);
        return true;
    }

    void up(Button b, uint32_t now)
    {
        const int i = static_cast<int>(b);
        if (!mDown[i]) {
            return;
        }
        mDown[i] = false;
        mHost.buttonUp(b, now - mDownAt[i] < kClickMaxMs);
    }

    void schedule(Button b, uint32_t at)
    {
        Release r { at, b, mPressId[static_cast<int>(b)] };
        auto pos = std::upper_bound(mReleases.begin(), mReleases.end(), r,
                                    [](const Release& x, const Release& y) { return x.at < y.at; });
        mReleases.insert(pos, r);
    }

    void stopRun()
    {
        if (mStopped) {
            return;
        }
        mStopped = true;
        if (Options::get().automated()) {
            Options::get().armShutdownWatchdog();
        }
        mHost.endRun();
    }

    ScriptHost&           mHost;
    std::vector<Step>     mSteps;
    size_t                mNext = 0;
    std::vector<Release>  mReleases;
    std::filesystem::path mScriptDir;
    bool                  mHasExit = false;
    bool                  mStarted = false;
    bool                  mStopped = false;
    std::chrono::steady_clock::time_point mStart;
    bool                  mDown[4]   = {};
    uint32_t              mDownAt[4] = {};
    uint32_t              mPressId[4] = {};
};

} // namespace SDK::Simulator::Automation

#endif // SDK_SIMULATOR_AUTOMATION_SCRIPT_HPP
