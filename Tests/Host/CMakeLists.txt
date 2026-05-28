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
    "${UNA_SDK_ROOT}/Examples/Apps/Running/Software/Libs/Sources/SettingsSerializer.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK_ROOT}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/core_json.c"
)

target_include_directories(una-sdk-host-tests PRIVATE
    "${UNA_SDK_ROOT}/Libs/Header"
    "${UNA_SDK_ROOT}/ThirdParty/coreJSON/source/include"
    "${UNA_SDK_ROOT}/Examples/Apps/Running/Software/Libs/Header"
)

target_link_libraries(una-sdk-host-tests PRIVATE
    sdk_host_test_support
    GTest::gtest_main
    GTest::gmock
)

enable_testing()
add_test(NAME una-sdk-host-tests COMMAND una-sdk-host-tests)
