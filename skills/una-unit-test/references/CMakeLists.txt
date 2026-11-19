# Host unit tests for a UNA Watch app, built with the PC's own compiler.
# Place this file in the app's Tests/ folder (beside Software/ and Resources/).
#
#   cmake -S Tests -B build-tests -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build-tests
#   ctest --test-dir build-tests --output-on-failure          # add -C Debug on Windows
#
# Needs UNA_SDK set. GoogleTest is downloaded on the first configure.
cmake_minimum_required(VERSION 3.21)
project(app-host-tests CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("$ENV{UNA_SDK}" STREQUAL "")
    message(FATAL_ERROR "UNA_SDK environment variable is not set. Point it at the SDK root.")
endif()
file(TO_CMAKE_PATH "$ENV{UNA_SDK}" UNA_SDK)

set(APP_LIBS "${CMAKE_CURRENT_LIST_DIR}/../Software/Libs")

# GoogleTest, at the version the SDK's own tests use.
include("${UNA_SDK}/Tests/Host/cmake/FetchGoogleTest.cmake")

# The SDK's test doubles: KernelFixture gives a ready SDK::Kernel with a fake
# clock, logger and an in-memory file system. The JSON reader/writer and coreJSON
# are here because most app code that touches files uses them.
set(COREJSON_DIR "${UNA_SDK}/ThirdParty/coreJSON/source")
add_library(una_test_support STATIC
    "${UNA_SDK}/Tests/Host/support/KernelTestDoubles.cpp"
    "${UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"
    "${UNA_SDK}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${COREJSON_DIR}/core_json.c"
)
target_include_directories(una_test_support PUBLIC
    "${UNA_SDK}/Tests/Host/support"
    "${UNA_SDK}/Libs/Header"
    "${COREJSON_DIR}/include"
)

# The tests, and the app sources they exercise. List sources one by one: most
# of the service (Service.cpp, sensor and GUI glue) needs the real kernel and
# does not belong in a unit test.
add_executable(app-tests
    # Tests in this folder
    ExampleClass_test.cpp
    # App code under test
    "${APP_LIBS}/Sources/ExampleClass.cpp"
)
target_include_directories(app-tests PRIVATE
    "${APP_LIBS}/Header"
)

if(MSVC)
    target_compile_options(app-tests PRIVATE /W3 /utf-8)
else()
    target_compile_options(app-tests PRIVATE -Wall -Wextra)
endif()

find_package(Threads REQUIRED)
target_link_libraries(app-tests PRIVATE
    una_test_support
    GTest::gtest_main
    GTest::gmock
    Threads::Threads
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(app-tests)
