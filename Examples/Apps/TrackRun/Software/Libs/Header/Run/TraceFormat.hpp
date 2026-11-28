/*******************************************************************************
 * @file   TraceFormat.hpp
 * @brief  Run: the lines of the GPS trace CSV (docs/GPS_TRACE_FORMAT.md,
 *         version 2), as the watch writes them. The host tests
 *         read them back with Tests/Host/TraceCsv.
 *
 *         Numbers are formatted from integers in fixed decimals, so the
 *         watch needs no float printf and the PC and the watch write the
 *         same bytes. Each call writes one whole line ending in "\n" into
 *         buf and returns its length, or 0 if it does not fit.
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_TRACE_FORMAT_HPP
#define RUN_TRACE_FORMAT_HPP

#include <cstddef>
#include <cstdint>
#include <limits>

namespace Run {
namespace TraceFormat {

constexpr int    kVersion     = 2;
constexpr size_t kMaxLineBytes = 160;

/** One reading and what the watch made of it. NaN (or a negative hdop, or
 *  a negative lap) writes a blank field. */
struct Row {
    uint32_t    tMs        = 0;
    bool        fix        = false;
    double      lat        = 0.0;
    double      lon        = 0.0;
    float       altM       = std::numeric_limits<float>::quiet_NaN();
    float       hdop       = -1.0f;
    float       gpsDistM   = std::numeric_limits<float>::quiet_NaN();
    float       trackDistM = std::numeric_limits<float>::quiet_NaN();
    int32_t     lap        = -1;
    const char* mode       = nullptr; ///< pre, count, cal, pause
    uint8_t     lane       = 0;       ///< 0: blank
    const char* segment    = nullptr; ///< road or track
};

/** "# trackrun-gps-trace v2" */
size_t versionLine(char* buf, size_t cap) noexcept;

/** "# key=value" */
size_t metaLine(char* buf, size_t cap, const char* key, const char* value) noexcept;
size_t metaLine(char* buf, size_t cap, const char* key, uint32_t value) noexcept;

/** The column header. */
size_t columnsLine(char* buf, size_t cap) noexcept;

/** A data row. */
size_t rowLine(char* buf, size_t cap, const Row& r) noexcept;

/** An action, as a comment a replay can act on: "# key=t_ms" or
 *  "# key=t_ms,value". value < 0 is left out. */
size_t actionLine(char* buf, size_t cap, const char* key, uint32_t tMs, int32_t value = -1) noexcept;

/** A number in fixed decimals (0 to 9), with no trailing newline. Returns
 *  its length, or 0 if it does not fit. Non-finite writes nothing. */
size_t fixed(char* buf, size_t cap, double v, int decimals) noexcept;

} // namespace TraceFormat
} // namespace Run

#endif // RUN_TRACE_FORMAT_HPP
