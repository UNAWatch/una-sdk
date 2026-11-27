# Track Run host (PC) unit tests.
#
# The SDK's own host tests build these too (Tests/Host/CMakeLists.txt adds
# this folder), so `ctest` there runs them. On their own, from the TrackRun
# folder, with UNA_SDK set as for the watch builds:
#
#   cmake -S Tests/Host -B Tests/Host/build
#   cmake --build Tests/Host/build --config Debug
#   ctest --test-dir Tests/Host/build -C Debug --output-on-failure
#
# Every compiler and maths library gives the same random draws and the same
# generator maths (PortableRandom.hpp, PortableMath.hpp), so a test gets the
# same traces on MSVC, GCC and Clang, on Windows, Linux and ARM.
# TRACKRUN_SEED_SET=1 (2, 3, ...) runs every randomised test on another set
# of draws.
#
# The track engine (Software/Libs/*/Oval) and Run/ are plain C++17 with no
# SDK headers, so they are compiled here unchanged. TraceGen, Replay and
# TraceCsv are host-only test tools. The robustness sweep prints its lap
# time figures to the console:
#
#   Tests/Host/build/Debug/trackrun-host-tests.exe --gtest_filter=Sweep.*
#
# The replay fixture in data/ is regenerated with:
#
#   Tests/Host/build/Debug/trackrun-host-tests.exe --gtest_also_run_disabled_tests --gtest_filter=*WriteFixture*
cmake_minimum_required(VERSION 3.21)
project(trackrun-host-tests CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# UNA_SDK_ROOT is set when the SDK's host tests add this folder.
if(NOT DEFINED UNA_SDK_ROOT)
    if("$ENV{UNA_SDK}" STREQUAL "")
        message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
    endif()
    set(UNA_SDK_ROOT "$ENV{UNA_SDK}")
endif()
set(APP_LIBS "${CMAKE_CURRENT_LIST_DIR}/../../Software/Libs")

if(NOT TARGET GTest::gtest_main)
    include("${UNA_SDK_ROOT}/Tests/Host/cmake/FetchGoogleTest.cmake")
endif()

# Warnings for our code only, after GoogleTest is set up.
if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    # The same floating-point results on every compiler: no fused multiply-add.
    add_compile_options(-ffp-contract=off)
endif()
if(MINGW)
    add_link_options(-static)
endif()

add_executable(trackrun-host-tests
    PortableRandom_test.cpp
    LaneTable_test.cpp
    LaneSetting_test.cpp
    LocalFrame_test.cpp
    LapCounter_test.cpp
    Sweep_test.cpp
    TrackModel_test.cpp
    Calibrator_test.cpp
    Tracker_test.cpp
    TrackerSweep_test.cpp
    TraceReplay_test.cpp
    TrackStore_test.cpp
    ReturnVisit_test.cpp
    Hdop_test.cpp
    Session_test.cpp
    SessionSweep_test.cpp
    StartSweep_test.cpp
    NeighbourSweep_test.cpp
    RunModules_test.cpp
    TrackSource_test.cpp
    WatchTrace_test.cpp
    TraceGen.cpp
    Replay.cpp
    TraceCsv.cpp

    "${APP_LIBS}/Sources/Oval/LaneTable.cpp"
    "${APP_LIBS}/Sources/Oval/LocalFrame.cpp"
    "${APP_LIBS}/Sources/Oval/LapCounter.cpp"
    "${APP_LIBS}/Sources/Oval/TrackModel.cpp"
    "${APP_LIBS}/Sources/Oval/Calibrator.cpp"
    "${APP_LIBS}/Sources/Oval/LapTiming.cpp"
    "${APP_LIBS}/Sources/Oval/Tracker.cpp"
    "${APP_LIBS}/Sources/Oval/TrackStore.cpp"
    "${APP_LIBS}/Sources/Oval/Session.cpp"
    "${APP_LIBS}/Sources/Run/SensorClock.cpp"
    "${APP_LIBS}/Sources/Run/PaceWindow.cpp"
    "${APP_LIBS}/Sources/Run/WeakGps.cpp"
    "${APP_LIBS}/Sources/Run/TickLog.cpp"
    "${APP_LIBS}/Sources/Run/TraceFormat.cpp"
    "${APP_LIBS}/Sources/Run/TrackSource.cpp"
)
target_include_directories(trackrun-host-tests PRIVATE
    "${APP_LIBS}/Header"
)
target_link_libraries(trackrun-host-tests PRIVATE GTest::gtest_main)
target_compile_definitions(trackrun-host-tests PRIVATE
    TRACE_DATA_DIR="${CMAKE_CURRENT_LIST_DIR}/data")
if(MSVC)
    target_compile_definitions(trackrun-host-tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()
add_test(NAME trackrun-host-tests COMMAND trackrun-host-tests --gtest_filter=-Targets.*)
# The accuracy targets: slow, and a report rather than a pass or fail of the
# code. Run with TRACKRUN_TARGETS set (ctest then includes them).
add_test(NAME trackrun-host-targets COMMAND trackrun-host-tests --gtest_filter=Targets.*)
set_tests_properties(trackrun-host-targets PROPERTIES LABELS targets)
file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake"
    "if(NOT DEFINED ENV{TRACKRUN_TARGETS})\n"
    "    set(CTEST_CUSTOM_TESTS_IGNORE trackrun-host-targets)\n"
    "endif()\n")

# The service's files with the SDK's in-memory kernel (Tests/Host/support).
set(SDK_TESTS "${UNA_SDK_ROOT}/Tests/Host")
set(COREJSON_DIR "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source")
add_executable(trackrun-service-tests
    ServiceFiles_test.cpp

    "${SDK_TESTS}/support/KernelTestDoubles.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/UnaLogger/Logger.cpp"

    "${APP_LIBS}/Sources/SettingsSerializer.cpp"
    "${APP_LIBS}/Sources/ActivitySummarySerializer.cpp"
    "${APP_LIBS}/Sources/TrackStoreFile.cpp"
    "${APP_LIBS}/Sources/GpsTraceWriter.cpp"
    "${APP_LIBS}/Sources/Run/TraceFormat.cpp"
    "${APP_LIBS}/Sources/Oval/LaneTable.cpp"
    "${APP_LIBS}/Sources/Oval/LocalFrame.cpp"
    "${APP_LIBS}/Sources/Oval/TrackModel.cpp"
    "${APP_LIBS}/Sources/Oval/TrackStore.cpp"

    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${COREJSON_DIR}/core_json.c"
    "${UNA_SDK_ROOT}/Libs/Source/Wrappers/StdLibWrappers.c"
)
target_include_directories(trackrun-service-tests PRIVATE
    "${APP_LIBS}/Header"
    "${SDK_TESTS}"
    "${SDK_TESTS}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${COREJSON_DIR}/include"
)
target_link_libraries(trackrun-service-tests PRIVATE GTest::gtest_main)
if(MSVC)
    target_compile_definitions(trackrun-service-tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
add_test(NAME trackrun-service-tests COMMAND trackrun-service-tests)
set_tests_properties(trackrun-service-tests PROPERTIES LABELS service)
