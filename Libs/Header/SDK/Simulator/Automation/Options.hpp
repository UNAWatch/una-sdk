/**
 * @file Options.hpp
 * @brief Command-line options for scripted and headless simulator runs.
 *
 * Simulator only. Every simulator (TouchGFX app, LVGL app, glance) reads the
 * same options, from its own command line and from the UNA_SIM_ARGS
 * environment variable (for launches from an IDE, which add nothing to the
 * command line):
 *
 *     --script FILE      run the timeline in FILE (see Script.hpp)
 *     --headless         no window; frames are still rendered and captured
 *     --duration-ms N    stop after N ms of watch time
 *     --out DIR          screenshots and sim.log go here (default: "sim-out")
 *     --fs-root DIR      the mock file system's root (default: the simulator's
 *                        own, relative to the working directory)
 *
 * The options are read from the process command line itself, so a simulator's
 * main() needs no change to accept them. Header-only, so no project file has to
 * list a new source.
 *
 * UNA_SIM_ARGS is split at spaces; put an argument that contains spaces in
 * double quotes ("C:\My Scripts\run.txt"). Backslashes are literal.
 *
 * Exit codes: 0 success, 1 a script step failed (a screenshot could not be
 * written, a replay file could not be read, a button event was lost), 2 bad
 * options or script, 3 the simulator did not shut down in time after it was
 * told to stop (whatever else happened before), 4 the app ran out of memory
 * (an LVGL app's pool, see SDK/Simulator/LVGL/LvglPool.hpp).
 */

#ifndef SDK_SIMULATOR_AUTOMATION_OPTIONS_HPP
#define SDK_SIMULATOR_AUTOMATION_OPTIONS_HPP

#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(__APPLE__)
#include <crt_externs.h>
#endif

namespace SDK::Simulator::Automation
{

/// Exit codes of a scripted run.
enum ExitCode : int {
    kExitOk          = 0,
    kExitStepFailed  = 1,
    kExitBadInput    = 2,
    kExitHung        = 3,
    kExitOutOfMemory = 4,
};

namespace detail
{
/// The run's exit code. Kept outside Options, which is destroyed at exit
/// before the atexit handler that reads this runs. Trivially destructible.
inline std::atomic<int>& exitCodeStore()
{
    static std::atomic<int> sCode { 0 };
    return sCode;
}
} // namespace detail

class Options
{
public:
    /// The options of this process, read on first use.
    static Options& get()
    {
        static Options sOptions;
        return sOptions;
    }

    bool               headless() const   { return mHeadless; }
    uint32_t           durationMs() const { return mDurationMs; }
    const std::string& scriptPath() const { return mScriptPath; }
    bool               scripted() const   { return !mScriptPath.empty(); }

    /// True when a run must end on its own: headless, or scripted.
    bool automated() const { return mHeadless || scripted() || mDurationMs != 0; }

    /// Arguments that are not options of this file, for hosts that take none of their own.
    const std::vector<std::string>& unknownArgs() const { return mUnknown; }

    /// Mock file system root: --fs-root, else the simulator's own default.
    /// Ends with a separator, as the mock file system expects.
    const char* fsRoot(const char* fallback)
    {
        if (mFsRoot.empty()) {
            return fallback;
        }
        return mFsRoot.c_str();
    }

    /// A path inside the output directory (made when an automated run starts,
    /// else on first use). Folders in the name are made too, so a screenshot
    /// named "shots/boot" can be written. A failure shows when the file is
    /// opened.
    std::string outPath(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        std::error_code ec;
        if (!mOutReady) {
            std::filesystem::create_directories(mOutDir, ec);
            mOutReady = true;
        }
        const std::filesystem::path path = std::filesystem::path(mOutDir) / name;
        std::filesystem::create_directories(path.parent_path(), ec);
        return path.string();
    }

    /// Write a line to sim.log in the output directory (automated runs only).
    void log(const char* text)
    {
        if (!automated() || text == nullptr) {
            return;
        }
        std::FILE* f = logFile();
        if (f == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(mMutex);
        std::fputs(text, f);
        std::fflush(f);
    }

    /// printf to stderr and sim.log: messages about the run itself.
    void report(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        std::fprintf(stderr, "[sim] %s\n", buffer);
        std::fflush(stderr);
        std::string line = std::string("[sim] ") + buffer + "\n";
        log(line.c_str());
    }

    /// A step failed: report it, and make the process exit with kExitStepFailed.
    void fail(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        report("FAILED: %s", buffer);
        int expected = kExitOk;
        detail::exitCodeStore().compare_exchange_strong(expected, kExitStepFailed);
    }

    /// Bad options or script: report and end the process now, before anything runs.
    [[noreturn]] void abortRun(const char* format, ...)
    {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        std::vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        report("ERROR: %s", buffer);
        std::_Exit(kExitBadInput);
    }

    int exitCode() const { return detail::exitCodeStore().load(); }

    /// The run was told to stop. If the process has not exited within the grace
    /// period (a thread that never returns), end it with kExitHung.
    void armShutdownWatchdog(uint32_t graceMs = 15000)
    {
        if (mWatchdogArmed.exchange(true)) {
            return;
        }
        std::thread([graceMs] {
            std::this_thread::sleep_for(std::chrono::milliseconds(graceMs));
            // stderr only: the hang may be a thread stuck holding the log's lock.
            // Written from another thread, so a stderr pipe nobody reads can't
            // keep the process alive: the exit follows after a short wait.
            try {
                std::thread([graceMs] {
                    std::fprintf(stderr, "[sim] ERROR: the simulator did not shut down within %u ms of the stop\n", graceMs);
                    std::fflush(stderr);
                }).detach();
            } catch (...) {
                // No thread to spare: exit without the message.
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::_Exit(kExitHung);
        }).detach();
    }

    /// Watch time in ms for a wall-clock interval: divided by how much the host
    /// slows its frames (ScriptHost::frameSlowdown()), so a script keeps its
    /// place against the GUI.
    static uint32_t toWatchMs(std::chrono::steady_clock::duration wall, double slowdown = 1.0)
    {
        const double ms = std::chrono::duration<double, std::milli>(wall).count() / slowdown;
        // Saturated: past UINT32_MAX ms the conversion would be undefined.
        if (!(ms < static_cast<double>(UINT32_MAX))) {
            return UINT32_MAX;
        }
        return ms > 0.0 ? static_cast<uint32_t>(ms) : 0;
    }

    static double slowFactor()
    {
        if (const char* slow = std::getenv("UNA_SIM_SLOW")) {
            const double factor = std::atof(slow);
            if (factor >= 1.0 && factor <= 10.0) {
                return factor;
            }
        }
        return 1.0;
    }

private:
    Options()
    {
        std::vector<std::string> args = processArgs();
        if (const char* env = std::getenv("UNA_SIM_ARGS")) {
            for (std::string& word : splitArgs(env)) {
                args.push_back(std::move(word));
            }
        }
        parse(args);
        if (automated()) {
            // The output directory and its sim.log must be usable before the
            // run starts, not found missing at the end.
            std::error_code ec;
            std::filesystem::create_directories(mOutDir, ec);
            if (!std::filesystem::is_directory(mOutDir, ec)) {
                std::fprintf(stderr, "[sim] ERROR: --out %s is not a directory and cannot be made one\n",
                             mOutDir.c_str());
                std::_Exit(kExitBadInput);
            }
            if (logFile() == nullptr) {
                std::fprintf(stderr, "[sim] ERROR: cannot write the run log in %s\n", mOutDir.c_str());
                std::_Exit(kExitBadInput);
            }
            // A failed step is reported however main() returns. Registered
            // while Options is still being built, so the handler runs after
            // Options is destroyed: it reads only the separate exit code.
            std::atexit([] {
                const int code = detail::exitCodeStore().load();
                if (code != kExitOk) {
                    std::_Exit(code);
                }
            });
        }
    }

    void parse(const std::vector<std::string>& args)
    {
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string& a = args[i];
            auto value = [&](const char* name) -> std::string {
                if (i + 1 >= args.size()) {
                    abortRun("%s needs a value", name);
                }
                return args[++i];
            };
            if (a == "--headless") {
                mHeadless = true;
            } else if (a == "--script") {
                mScriptPath = value("--script");
                if (mScriptPath.empty()) {
                    abortRun("--script needs a file");
                }
            } else if (a == "--out") {
                // Checked before it is kept: the error report itself writes
                // sim.log there.
                std::string out = value("--out");
                if (out.empty()) {
                    abortRun("--out needs a directory");
                }
                mOutDir = std::move(out);
            } else if (a == "--fs-root") {
                std::string root = value("--fs-root");
                if (root.empty()) {
                    abortRun("--fs-root needs a directory");
                }
                std::error_code ec;
                std::filesystem::create_directories(root, ec);
                if (!std::filesystem::is_directory(root, ec)) {
                    abortRun("--fs-root %s is not a directory and cannot be made one", root.c_str());
                }
                if (root.back() != '/' && root.back() != '\\') {
                    root += '/';
                }
                mFsRoot = root;
            } else if (a == "--duration-ms") {
                const std::string v = value("--duration-ms");
                char* end = nullptr;
                errno = 0;
                const unsigned long long n = std::strtoull(v.c_str(), &end, 10);
                if (v.empty() || v[0] < '0' || v[0] > '9' || *end != '\0' || errno == ERANGE ||
                    n == 0 || n > UINT32_MAX) {
                    abortRun("--duration-ms needs a whole number from 1 to %u, got '%s'", UINT32_MAX, v.c_str());
                }
                mDurationMs = static_cast<uint32_t>(n);
            } else {
                mUnknown.push_back(a);
            }
        }
    }

    /// UNA_SIM_ARGS split at spaces; double quotes group, backslashes are literal.
    static std::vector<std::string> splitArgs(const char* text)
    {
        std::vector<std::string> out;
        std::string word;
        bool inWord = false;
        bool quoted = false;
        for (const char* p = text; *p != '\0'; ++p) {
            if (*p == '"') {
                quoted = !quoted;
                inWord = true;
            } else if ((*p == ' ' || *p == '\t') && !quoted) {
                if (inWord) {
                    out.push_back(word);
                    word.clear();
                    inWord = false;
                }
            } else {
                word += *p;
                inWord = true;
            }
        }
        if (inWord) {
            out.push_back(word);
        }
        return out;
    }

    /// The process's own arguments, without the program name.
    static std::vector<std::string> processArgs()
    {
        std::vector<std::string> args;
#if defined(_WIN32)
        // Set by the C runtime for main() and WinMain() programs alike. It is
        // main()'s own argv, so a program that removes arguments from it
        // (GoogleTest does) leaves null entries below __argc.
        for (int i = 1; i < __argc && __argv[i] != nullptr; ++i) {
            args.emplace_back(__argv[i]);
        }
#elif defined(__APPLE__)
        const int    argc = *_NSGetArgc();
        char** const argv = *_NSGetArgv();
        for (int i = 1; i < argc && argv[i] != nullptr; ++i) {
            args.emplace_back(argv[i]);
        }
#elif defined(__linux__)
        std::ifstream in("/proc/self/cmdline", std::ios::binary);
        std::string arg;
        bool first = true;
        while (std::getline(in, arg, '\0')) {
            if (!first) {
                args.push_back(arg);
            }
            first = false;
        }
#endif
        return args;
    }

    std::FILE* logFile()
    {
        std::call_once(mLogOnce, [this] {
            const std::string path = outPath("sim.log");
            mLog = std::fopen(path.c_str(), "w");
            if (mLog == nullptr) {
                std::fprintf(stderr, "[sim] warning: cannot write %s: %s\n", path.c_str(), std::strerror(errno));
            }
        });
        return mLog;
    }

    bool                     mHeadless   = false;
    uint32_t                 mDurationMs = 0;
    std::string              mScriptPath;
    std::string              mOutDir = "sim-out";
    std::string              mFsRoot;
    std::vector<std::string> mUnknown;

    std::mutex        mMutex;
    bool              mOutReady = false;
    std::FILE*        mLog      = nullptr;
    std::once_flag    mLogOnce;
    std::atomic<bool> mWatchdogArmed { false };
};

} // namespace SDK::Simulator::Automation

#endif // SDK_SIMULATOR_AUTOMATION_OPTIONS_HPP
