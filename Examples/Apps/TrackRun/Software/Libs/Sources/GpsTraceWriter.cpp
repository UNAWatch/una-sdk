/**
 ******************************************************************************
 * @file    GpsTraceWriter.cpp
 * @brief   The raw GPS trace CSV, one per run (GpsTraceWriter.hpp).
 ******************************************************************************
 */

#include "GpsTraceWriter.hpp"

#include <cstdio>
#include <cstring>

#define LOG_MODULE_PRX      "GpsTrace"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace TF = Run::TraceFormat;

void GpsTraceWriter::preRun(const TF::Row& r)
{
    mPre[(mPreHead + mPreCount) % kPreRows] = r;
    if (mPreCount < kPreRows) {
        ++mPreCount;
    } else {
        mPreHead = static_cast<uint8_t>((mPreHead + 1u) % kPreRows);
    }
}

bool GpsTraceWriter::put(size_t len)
{
    if (!mFile) {
        return false;
    }
    size_t wrote = 0;
    if (len == 0 || !mFile->write(mLine, len, wrote) || wrote != len) {
        LOG_ERROR("Write failed: trace stopped [%s]\n", mPath);
        mFile->close(); // the file stays this run's: discard() still removes it
        mFile.reset();
        return false;
    }
    return true;
}

bool GpsTraceWriter::open(const char* fitPath, uint32_t startMs, uint8_t lane, const char* appVersion,
                          const char* storeText, size_t storeLen, float hdopScale)
{
    close();
    mRows = 0;
    // Activity/YYYYMM/activity_<stamp>.fit -> Activity/YYYYMM/trace_<stamp>.csv
    const char* slash = fitPath != nullptr ? std::strrchr(fitPath, '/') : nullptr;
    const char* stamp = slash != nullptr ? std::strstr(slash, "activity_") : nullptr;
    const char* dot   = stamp != nullptr ? std::strrchr(stamp, '.') : nullptr;
    // On a failed open the pre-run readings go too: they are this start's,
    // not the next one's.
    if (stamp == nullptr || dot == nullptr) {
        LOG_ERROR("No FIT file to pair the trace with\n");
        clearPreRun();
        return false;
    }
    const int n = std::snprintf(mPath, sizeof(mPath), "%.*strace_%.*s.csv", static_cast<int>(slash + 1 - fitPath),
                                fitPath, static_cast<int>(dot - (stamp + 9)), stamp + 9);
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(mPath)) {
        clearPreRun();
        return false;
    }
    mFile = mKernel.fs.file(mPath);
    if (!mFile || !mFile->open(true, true)) {
        LOG_ERROR("Cannot create [%s]\n", mPath);
        mFile.reset();
        clearPreRun();
        return false;
    }
    mCreated = true;

    bool ok = put(TF::versionLine(mLine, sizeof(mLine)));
    ok = ok && put(TF::metaLine(mLine, sizeof(mLine), "start_ms", startMs));
    ok = ok && put(TF::metaLine(mLine, sizeof(mLine), "lane", static_cast<uint32_t>(lane)));
    ok = ok && put(TF::metaLine(mLine, sizeof(mLine), "table", "outdoor"));
    ok = ok && put(TF::metaLine(mLine, sizeof(mLine), "app_version", appVersion));
    if (ok && hdopScale > 0.0f) {
        char v[16];
        const size_t len = TF::fixed(v, sizeof(v) - 1u, hdopScale, 4);
        v[len] = '\0';
        ok = put(TF::metaLine(mLine, sizeof(mLine), "hdop_scale", v));
        ok = ok && put(TF::metaLine(mLine, sizeof(mLine), "hdop_note",
                                    "simulator: the hdop column is as the sensor gave it; multiply by hdop_scale"));
    }
    // The stored tracks the run started with, so a replay can rebuild them
    // (the header line is left out; each track line is one meta line).
    for (size_t i = 0; ok && storeText != nullptr && i < storeLen;) {
        const char* nl  = static_cast<const char*>(std::memchr(storeText + i, '\n', storeLen - i));
        const size_t end = nl != nullptr ? static_cast<size_t>(nl - storeText) : storeLen;
        if (end - i > 6 && std::strncmp(storeText + i, "track ", 6) == 0) {
            char line[TF::kMaxLineBytes + 300];
            const size_t l = end - i < sizeof(line) - 1u ? end - i : sizeof(line) - 1u;
            std::memcpy(line, storeText + i, l);
            line[l] = '\0';
            ok = put(TF::metaLine(mLine, sizeof(mLine), "store", line));
        }
        i = end + 1u;
    }
    ok = ok && put(TF::columnsLine(mLine, sizeof(mLine)));
    for (uint8_t k = 0; ok && k < mPreCount; ++k) {
        ok = put(TF::rowLine(mLine, sizeof(mLine), mPre[(mPreHead + k) % kPreRows]));
    }
    clearPreRun();
    if (!ok) {
        return false;
    }
    mFile->flush();
    mLastFlush = mKernel.sys.getTimeMs();
    LOG_INFO("GPS trace [%s]\n", mPath);
    return true;
}

void GpsTraceWriter::maybeFlush()
{
    const uint32_t now = mKernel.sys.getTimeMs();
    if (mFile && now - mLastFlush >= kFlushMs) {
        mFile->flush();
        mLastFlush = now;
    }
}

void GpsTraceWriter::row(const TF::Row& r)
{
    if (!mFile) {
        return;
    }
    if (put(TF::rowLine(mLine, sizeof(mLine), r))) {
        ++mRows;
        maybeFlush();
    }
}

void GpsTraceWriter::action(const char* key, uint32_t tMs, int32_t value)
{
    if (mFile) {
        put(TF::actionLine(mLine, sizeof(mLine), key, tMs, value));
    }
}

void GpsTraceWriter::close()
{
    mCreated = false; // kept: discard() leaves it
    if (!mFile) {
        return;
    }
    mFile->flush();
    mFile->close();
    mFile.reset();
    LOG_INFO("GPS trace closed: %u rows\n", static_cast<unsigned>(mRows));
}

void GpsTraceWriter::discard()
{
    // Also a file a write error already closed.
    if (mFile) {
        mFile->close();
        mFile.reset();
    }
    if (mCreated) {
        mKernel.fs.remove(mPath);
        LOG_INFO("GPS trace deleted with the run\n");
    }
    mCreated = false;
}
