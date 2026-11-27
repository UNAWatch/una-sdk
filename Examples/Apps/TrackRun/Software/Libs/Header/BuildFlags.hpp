/*******************************************************************************
 * @file   BuildFlags.hpp
 * @brief  What a build kind turns on. TrackRun-CMake sets TRACKRUN_BUILD:
 *         "release" (the default) or "tester", which also writes a GPS trace
 *         beside each activity (docs/GPS_TRACE_FORMAT.md).
 ******************************************************************************/

#ifndef BUILD_FLAGS_HPP
#define BUILD_FLAGS_HPP

#if defined(TRACKRUN_TESTER_BUILD) && TRACKRUN_TESTER_BUILD
#define TRACKRUN_GPS_TRACE_EN 1
#else
#define TRACKRUN_GPS_TRACE_EN 0
#endif

#endif // BUILD_FLAGS_HPP
