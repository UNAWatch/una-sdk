cmake_minimum_required(VERSION 3.21)
project(una-sdk-host-tests CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile commands for host tests" FORCE)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(UNA_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/FetchGoogleTest.cmake)

# coreJSON backs the SDK JSON reader/writer (Running settings + calibrator store).
set(COREJSON_DIR "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source")
if(NOT IS_DIRECTORY "${COREJSON_DIR}/include")
    message(FATAL_ERROR
        "coreJSON sources not found at ${COREJSON_DIR}. "
        "Initialize submodules: git submodule update --init --recursive"
    )
endif()

add_library(sdk_host_test_support STATIC
    support/KernelTestDoubles.cpp
    "${UNA_SDK_ROOT}/Libs/Source/UnaLogger/Logger.cpp"
)

target_include_directories(sdk_host_test_support PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/support"
    "${UNA_SDK_ROOT}/Libs/Header"
)

add_executable(una-sdk-host-tests
    apps/Running/Settings_test.cpp
    apps/Running/SettingsSerializer_test.cpp
    calibration/OutdoorStrideCalibrator_test.cpp
    calibration/StrideLut_test.cpp
    calibration/StrideMath_test.cpp
    calibration/CadenceStrideModel_test.cpp
    calibration/TreadmillSpeedEstimator_test.cpp
    calibration/CadenceSpikeFilter_test.cpp
    fit/FitWriter_test.cpp
    fit/FitProfile_test.cpp
    fit/FitRoundTrip_test.cpp
    apps/Running/ActivityWriter_test.cpp
    sensorlayer/HeartRateParser_test.cpp
    sensorlayer/HeartRateExParser_test.cpp
    "${UNA_SDK_ROOT}/Examples/Apps/Running/Software/Libs/Sources/SettingsSerializer.cpp"
    "${UNA_SDK_ROOT}/Examples/Apps/Running/Software/Libs/Sources/ActivityWriter.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Calibration/OutdoorStrideCalibrator.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Calibration/StrideLut.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Calibration/CadenceStrideModel.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Calibration/TreadmillSpeedEstimator.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitCrc.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitWriter.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/Fit/FitRecordCadence.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${COREJSON_DIR}/core_json.c"
)
# NOTE: the FitHelper cadence-encode / field-description tests are dropped here
# while the FIT encode path is reimplemented natively (SDK::Fit). They will be
# replaced by native equivalents once FitHelper is ported onto FitWriter.

target_include_directories(una-sdk-host-tests PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${UNA_SDK_ROOT}/Libs/Header"
    "${COREJSON_DIR}/include"
    "${UNA_SDK_ROOT}/Examples/Apps/Running/Software/Libs/Header"
)

target_link_libraries(una-sdk-host-tests PRIVATE
    sdk_host_test_support
    GTest::gtest_main
    GTest::gmock
)

enable_testing()
add_test(NAME una-sdk-host-tests COMMAND una-sdk-host-tests)
