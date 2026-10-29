cmake_minimum_required(VERSION 3.21)
project(GlanceStepsSimulator C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
include($ENV{UNA_SDK}/cmake/una-glance-simulator.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../Libs/libs.cmake)

una_add_glance_simulator(GlanceStepsSimulator
    APP_ID "8899AABBCCDDEEFF"
    SOURCES ${LIBS_SOURCES}
    INCLUDE_DIRS ${LIBS_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}
)

include(CTest)
if(BUILD_TESTING)
    add_executable(GlanceHostTests "$ENV{UNA_SDK}/Tests/Host/simulator/GlanceHost_test.cpp")
    target_link_libraries(GlanceHostTests PRIVATE GlanceStepsSimulatorRuntime)
    una_simulator_link_sdl2(GlanceHostTests)
    add_test(NAME GlanceHostRendering COMMAND GlanceHostTests)
    set_tests_properties(GlanceHostRendering PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy" TIMEOUT 10)
    add_test(NAME GlanceStepsLifecycle COMMAND GlanceStepsSimulator
        --duration-ms 2500 --screenshot "${CMAKE_CURRENT_BINARY_DIR}/glance-steps.bmp")
    set_tests_properties(GlanceStepsLifecycle PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy" TIMEOUT 15
        PASS_REGULAR_EXPRESSION "[2-9][0-9]* updates captured|[1-9][0-9]+ updates captured")
endif()
