# UNA SDK -- PC simulator support.
#
# A simulator runs an app's real service and GUI processes on the developer's
# machine against the SDK's mock kernel (Libs/Source/Simulator). The TouchGFX
# apps build theirs from simulator/msvs and simulator/gcc; this file gives a
# CMake project the same source lists, plus the LVGL host window.
#
#   UNA_SDK_SOURCES_SIMULATOR          mock kernel, simulated OS, messaging and sensors
#   UNA_SDK_SOURCES_SIMULATOR_SERVICE  SDK libraries a service process uses, host-buildable
#   UNA_SDK_SOURCES_SIMULATOR_LVGL     the LVGL port, its SDL2 host and LVGL itself
#   UNA_SDK_INCLUDE_DIRS_SIMULATOR
#   UNA_SDK_DEFINES_SIMULATOR
#   una_simulator_link_sdl2(<target>)  find SDL2 (or use the copy TouchGFX ships on Windows)
#
# Requires UNA_SDK in the environment, like una-sdk.cmake, which it includes.

include(${CMAKE_CURRENT_LIST_DIR}/una-sdk.cmake)

set(UNA_SDK_SOURCES_SIMULATOR
    "$ENV{UNA_SDK}/Libs/Source/Simulator/App/AppMessageCore.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/App/DualAppComm.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/App/KernelMessageDispatcher.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/App/MessageManager.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/ComponentSimulator.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/InstanceSensorLayer.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/SampleRateAdapter.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/SensorDataQueue.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/SensorDataSample.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/SensorDriver.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/SensorListener.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/SensorManager.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Gps/GpsAltimeter.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Gps/GpsDistance.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Gps/GpsLocation.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Gps/GpsSpeed.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/HeartRate/SensorHeartRate.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/HeartRate/SensorHeartRateMetrics.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Imu/ImuRunningCadence.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Imu/ImuStepCounter.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/Imu/ImuWristMotion.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/SensorBatteryLevel.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Sensors/SensorPressure.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Components/Simulator/GpsStepCounterSimulator.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Kernel/Kernel.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Kernel/Mock/Backlight.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Kernel/Mock/Buzzer.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Kernel/Mock/FileSystem.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Kernel/Mock/System.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/Kernel/Mock/Vibro.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/OS/OS.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/OS/OneShotTimer.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/OS/SwTimer.cpp"
)

# The service's SDK dependencies without the ARM start-up files: the same set
# the TouchGFX simulators list in simulator/gcc/Makefile.
set(UNA_SDK_SOURCES_SIMULATOR_SERVICE
    "$ENV{UNA_SDK}/Libs/Source/Kernel/KernelBuilder.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/GuiCommandProcessor.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Timer/Timer.cpp"
    "$ENV{UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Wrappers/StdLibWrappers.c"
    ${UNA_SDK_SOURCES_CALIBRATION}
    ${UNA_SDK_SOURCES_FIT}
    ${UNA_SDK_SOURCES_JSON}
    ${UNA_SDK_SOURCES_SENSOR}
    ${UNA_SDK_SOURCES_TRACKMAP}
)

# The LVGL GUI process as it runs on the watch (port + LVGL), plus the host
# that stands in for the kernel's display, ticks and buttons.
set(UNA_SDK_SOURCES_SIMULATOR_LVGL
    "$ENV{UNA_SDK}/Libs/Source/Port/LVGL/LvglPort.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Simulator/LVGL/LvglHost.cpp"
    ${UNA_SDK_LVGL_SOURCES}
)

set(UNA_SDK_INCLUDE_DIRS_SIMULATOR
    "$ENV{UNA_SDK}/Libs/Header"
    "$ENV{UNA_SDK}/ThirdParty/coreJSON/source/include"
)

# SIMULATOR selects the simulator paths in SDK sources; UNA_SIM_NO_TOUCHGFX
# keeps the mock kernel's logging and exit path off TouchGFX; SDL_MAIN_HANDLED
# lets the program keep its own main().
set(UNA_SDK_DEFINES_SIMULATOR
    SIMULATOR
    ENABLE_LOG
    UNA_SIM_NO_TOUCHGFX
    SDL_MAIN_HANDLED
)
if(MSVC)
    list(APPEND UNA_SDK_DEFINES_SIMULATOR
        WIN32
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING
    )
endif()

# Link SDL2 into <target>. An installed SDL2 (libsdl2-dev, vcpkg, ...) is used
# when CMake can find it; on Windows the copy TouchGFX ships is the fallback.
# That copy is 32-bit, so configure such a build with -A Win32.
function(una_simulator_link_sdl2 target)
    find_package(SDL2 CONFIG QUIET)
    if(TARGET SDL2::SDL2)
        target_link_libraries(${target} PRIVATE SDL2::SDL2)
        return()
    endif()
    if(SDL2_FOUND)
        target_include_directories(${target} PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(${target} PRIVATE ${SDL2_LIBRARIES})
        return()
    endif()
    if(WIN32)
        set(sdl_root "$ENV{UNA_SDK}/ThirdParty/touchgfx")
        if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
            message(FATAL_ERROR
                "No SDL2 found and the SDL2 shipped with TouchGFX is 32-bit. "
                "Configure a 32-bit build (cmake -A Win32 ...) or install SDL2 "
                "so that find_package(SDL2) succeeds.")
        endif()
        target_include_directories(${target} PRIVATE
            "${sdl_root}/framework/include/platform/hal/simulator/sdl2/vendor/SDL2")
        target_link_libraries(${target} PRIVATE "${sdl_root}/lib/sdl2/win32/SDL2.lib")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${sdl_root}/lib/sdl2/win32/SDL2.dll" "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying SDL2.dll next to ${target}")
        return()
    endif()
    message(FATAL_ERROR
        "SDL2 not found. Install its development package (Debian/Ubuntu: libsdl2-dev).")
endfunction()
