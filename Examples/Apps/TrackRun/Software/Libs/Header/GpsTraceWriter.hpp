/**
 ******************************************************************************
 * @file    GpsTraceWriter.hpp
 * @brief   The raw GPS trace: one CSV per run beside its FIT
 *          file, Activity/YYYYMM/trace_<same stamp>.csv, in the format of
 *          docs/GPS_TRACE_FORMAT.md version 2 (lines from Run::TraceFormat).
 *
 *          Opened at Start with the meta lines (start_ms, lane, table,
 *          app_version, hdop_scale in the simulator, and the stored tracks the
 *          run started with), then the readings primed before Start as "pre"
 *          rows. One row per GPS reading after that, and action lines (Lap
 *          presses, lanes set) a replay can repeat.
 *          Flushed every kFlushMs; on any write error it closes and stops,
 *          as RunLVGL's stride trace does.
 ******************************************************************************
 */

#ifndef GPS_TRACE_WRITER_HPP
#define GPS_TRACE_WRITER_HPP

#include "Run/TraceFormat.hpp"
#include "SDK/Kernel/Kernel.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

class GpsTraceWriter {
public:
    static constexpr uint32_t kFlushMs  = 10000;
    static constexpr uint8_t  kPreRows  = 10; ///< Pre-run readings kept for the file

    explicit GpsTraceWriter(SDK::Kernel& kernel) : mKernel(kernel) {}

    /** A pre-run reading, kept until open(). */
    void preRun(const Run::TraceFormat::Row& r);
    void clearPreRun() { mPreCount = 0; mPreHead = 0; }

    /** Opens the trace beside fitPath. storeText: the stored tracks as text
     *  (TrackStore::serialise), one meta line per track. hdopScale 0: none. */
    bool open(const char* fitPath, uint32_t startMs, uint8_t lane, const char* appVersion,
              const char* storeText, size_t storeLen, float hdopScale);

    void row(const Run::TraceFormat::Row& r);
    void action(const char* key, uint32_t tMs, int32_t value = -1);
    void close();
    /** Closes the trace and deletes it: the run was discarded. */
    void discard();

    bool        isOpen() const { return mFile != nullptr; }
    const char* path() const { return mPath; }
    uint32_t    rows() const { return mRows; }

private:
    bool put(size_t len);
    void maybeFlush();

    SDK::Kernel&                           mKernel;
    std::unique_ptr<SDK::Interface::IFile> mFile;
    char     mLine[Run::TraceFormat::kMaxLineBytes + 400] = {}; ///< Room for a store line
    char     mPath[SDK::Interface::IFileSystem::skMaxPathLen] = {};
    bool     mCreated = false; ///< mPath is this run's file, until close()
    uint32_t mLastFlush = 0;
    uint32_t mRows      = 0;
    Run::TraceFormat::Row mPre[kPreRows];
    uint8_t  mPreHead  = 0;
    uint8_t  mPreCount = 0;
};

#endif // GPS_TRACE_WRITER_HPP
