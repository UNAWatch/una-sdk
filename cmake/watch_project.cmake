# Watch SDK CMake Project Configuration
# This file contains common CMake logic extracted from app CMakeLists.txt
# to enable environment-variable based SDK referencing

# Validate UNA_SDK
if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR "UNA_SDK environment variable must be set")
endif()

set(UNA_SDK "$ENV{UNA_SDK}")

# Common toolchain setup
set(CMAKE_TOOLCHAIN_FILE "${UNA_SDK}/cmake/toolchain-arm-none-eabi.cmake")

# Set standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Common compile options (match CubeIDE exactly)
add_compile_options(
    -mcpu=cortex-m33
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -Os
    -fPIC
    -Wall
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
    -fno-rtti
    -fno-use-cxa-atexit
    -fstack-usage
    -nodefaultlibs
    -nostdlib
    -mthumb
)

# Common linker options
set(WATCH_COMMON_LINK_OPTIONS
    -Wl,--gc-sections
    -nostartfiles
    -nodefaultlibs
    -nostdlib
    -static
    -Wl,--emit-relocs
    -Wl,-L${UNA_SDK}/Utilities/Scripts/linker
    -mcpu=cortex-m33
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -mthumb
)

# Function to set up build version using git describe
function(watch_setup_version BUILD_VERSION_OUT)
    set(BUILD_VERSION "1.0.0-dev")  # Default fallback

    execute_process(COMMAND git rev-parse --git-dir OUTPUT_VARIABLE GIT_DIR ERROR_QUIET)
    if(GIT_DIR)
        execute_process(COMMAND git rev-parse --short=7 HEAD OUTPUT_VARIABLE COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(COMMAND git describe --exact-match --tags HEAD RESULT_VARIABLE TAG_RESULT OUTPUT_VARIABLE TAG OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(TAG_RESULT EQUAL 0)
            string(REGEX REPLACE "^v" "" TAG "${TAG}")
            set(BUILD_VERSION "${TAG}")
        else()
            execute_process(COMMAND git describe --always --tags --abbrev=7 OUTPUT_VARIABLE DESC OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            if(DESC)
                if(DESC MATCHES "^g")
                    string(REGEX REPLACE "^g" "" HASH "${DESC}")
                    set(BUILD_VERSION "${HASH}")
                else()
                    string(REGEX MATCH "(.+)-g" _ "${DESC}")
                    set(TAG "${CMAKE_MATCH_1}")
                    string(REGEX MATCH "g(.+)" _ "${DESC}")
                    set(HASH "${CMAKE_MATCH_1}")
                    string(REGEX REPLACE "^v" "" TAG "${TAG}")
                    set(BUILD_VERSION "${TAG}-${HASH}")
                endif()
            else()
                # Fallback to commit hash
                set(BUILD_VERSION "dev-${COMMIT_HASH}")
            endif()
        endif()
        execute_process(COMMAND git status --porcelain OUTPUT_VARIABLE GIT_STATUS)
        if(GIT_STATUS)
            set(BUILD_VERSION "${BUILD_VERSION}-dirty")
        endif()
    endif()

    set(${BUILD_VERSION_OUT} "${BUILD_VERSION}" PARENT_SCOPE)
    message("BUILD_VERSION: ${BUILD_VERSION}")
endfunction()

# Function to build service executable
function(watch_build_service TARGET_NAME LIBS_PATH TOUCHGFX_GUI_PATH)
    # Service sources - app-specific + common SDK sources
    set(SERVICE_SOURCES
        "${LIBS_PATH}/Sources/ActivitySummarySerializer.cpp"
        "${LIBS_PATH}/Sources/ActivityWriter.cpp"
        "${LIBS_PATH}/Sources/Service.cpp"
        "${LIBS_PATH}/Sources/SettingsSerializer.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/syscalls.cpp"

        "${UNA_SDK}/Libs/Source/AppSystem/AtExitImpl.cpp"
        "${UNA_SDK}/Libs/Source/AppSystem/EntryPoint/Service/main.cpp"
        "${UNA_SDK}/Libs/Source/AppSystem/startup_user_app.s"
        "${UNA_SDK}/Libs/Source/AppSystem/system.cpp"
        "${UNA_SDK}/Libs/Source/FitHelper/FitHelper.cpp"
        "${UNA_SDK}/Libs/Source/JSON/JsonStreamReader.cpp"
        "${UNA_SDK}/Libs/Source/JSON/JsonStreamWriter.cpp"
        "${UNA_SDK}/Libs/Source/Kernel/KernelBuilder.cpp"
        "${UNA_SDK}/Libs/Source/SensorLayer/SensorConnection.cpp"
        "${UNA_SDK}/Libs/Source/TrackMap/TrackMapBuilder.cpp"
        "${UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"

        "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit.c"
        "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_convert.c"
        "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_crc.c"
        "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_product.c"

        "${UNA_SDK}/ThirdParty/coreJSON/source/core_json.c"
    )

    add_executable(${TARGET_NAME} ${SERVICE_SOURCES})

    target_include_directories(${TARGET_NAME} PRIVATE
        "${LIBS_PATH}/Header"
        "${UNA_SDK}/Libs/Header"
        "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c"
        "${UNA_SDK}/ThirdParty/coreJSON/source/include"
        "${TOUCHGFX_GUI_PATH}/gui/include"
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE
        BUILD_VERSION="${BUILD_VERSION}"
        APP_NAME="${APP_NAME}"
        DEV_ID="${DEV_ID}"
        APP_ID="${APP_ID}"
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        -Wl,--start-group
        ${UNA_SDK}/libc++/libstdc++.a
        -Wl,--end-group
    )

    target_link_options(${TARGET_NAME} PRIVATE
        -T "${CMAKE_CURRENT_SOURCE_DIR}/RunningService.ld"
        -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.elf.map
        ${WATCH_COMMON_LINK_OPTIONS}
        -L${UNA_SDK}/libc++
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND python3 ${UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext srv
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Function to build GUI executable
function(watch_build_gui TARGET_NAME LIBS_PATH TOUCHGFX_GUI_PATH)
    file(GLOB_RECURSE GUI_SRC_SOURCES
        "${TOUCHGFX_GUI_PATH}/gui/src/*.cpp"
        "${TOUCHGFX_GUI_PATH}/generated/fonts/src/*.cpp"
        "${TOUCHGFX_GUI_PATH}/generated/gui_generated/src/*.cpp"
        "${TOUCHGFX_GUI_PATH}/generated/images/src/*.cpp"
        "${TOUCHGFX_GUI_PATH}/generated/texts/src/*.cpp"
    )

    set(GUI_SOURCES
        ${GUI_SRC_SOURCES}

        "${CMAKE_CURRENT_SOURCE_DIR}/PaintImpl.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/syscalls.cpp"

        "${UNA_SDK}/Libs/Source/AppSystem/AtExitImpl.cpp"
        "${UNA_SDK}/Libs/Source/AppSystem/EntryPoint/TouchGFX/main.cpp"
        "${UNA_SDK}/Libs/Source/AppSystem/startup_user_app.s"
        "${UNA_SDK}/Libs/Source/AppSystem/system.cpp"
        "${UNA_SDK}/Libs/Source/Kernel/KernelBuilder.cpp"
        "${UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"

        "${UNA_SDK}/Port/TouchGFX/STM32TouchController.cpp"
        "${UNA_SDK}/Port/TouchGFX/TouchGFXCommandProcessor.cpp"
        "${UNA_SDK}/Port/TouchGFX/TouchGFXGPIO.cpp"
        "${UNA_SDK}/Port/TouchGFX/TouchGFXHAL.cpp"

        "${UNA_SDK}/Port/TouchGFX/generated/OSWrappers.cpp"
        "${UNA_SDK}/Port/TouchGFX/generated/STM32DMA.cpp"
        "${UNA_SDK}/Port/TouchGFX/generated/TouchGFXConfiguration.cpp"
        "${UNA_SDK}/Port/TouchGFX/generated/TouchGFXGeneratedHAL.cpp"
    )

    add_executable(${TARGET_NAME} ${GUI_SOURCES})

    target_include_directories(${TARGET_NAME} PRIVATE
        "${LIBS_PATH}/Header"
        "${UNA_SDK}/Libs/Header"
        "${UNA_SDK}/Port/TouchGFX"
        "${UNA_SDK}/Port/TouchGFX/generated"
        "${TOUCHGFX_GUI_PATH}/touchgfx/framework/include"
        "${TOUCHGFX_GUI_PATH}/generated/fonts/include"
        "${TOUCHGFX_GUI_PATH}/generated/gui_generated/include"
        "${TOUCHGFX_GUI_PATH}/generated/images/include"
        "${TOUCHGFX_GUI_PATH}/generated/texts/include"
        "${TOUCHGFX_GUI_PATH}/generated/videos/include"
        "${TOUCHGFX_GUI_PATH}/gui/include"
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        -Wl,--start-group
        ${UNA_SDK}/libc++/libstdc++.a
        ${TOUCHGFX_GUI_PATH}/touchgfx/lib/core/cortex_m33/gcc/libtouchgfx-float-abi-hard.a
        -Wl,--end-group
    )

    target_link_options(${TARGET_NAME} PRIVATE
        -L${TOUCHGFX_GUI_PATH}/touchgfx/lib/core/cortex_m33/gcc
        -T "${CMAKE_CURRENT_SOURCE_DIR}/RunningGUI.ld"
        -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.elf.map
        ${WATCH_COMMON_LINK_OPTIONS}
        -L${UNA_SDK}/libc++
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND python3 ${UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext gui
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Main function to build a complete watch app
function(watch_build_app)
    # Set up paths relative to current source directory
    set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Libs")
    set(TOUCHGFX_GUI_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../TouchGFX-GUI")
    set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Output")
    set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../Resources")
    
    set(SCRIPTS_PATH "${UNA_SDK}/Utilities/Scripts")

    # Set up version
    watch_setup_version(BUILD_VERSION)

    # Make BUILD_VERSION available globally
    set(BUILD_VERSION "${BUILD_VERSION}" PARENT_SCOPE)

    # Build service and GUI
    watch_build_service(${APP_NAME}Service.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})
    watch_build_gui(${APP_NAME}GUI.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})

    # Final app merging
    add_custom_target(${APP_NAME}App ALL
        DEPENDS ${APP_NAME}Service.elf ${APP_NAME}GUI.elf
        COMMAND python3 ${SCRIPTS_PATH}/app_merging/app_merging.py -normal_icon ${RESOURCES_PATH}/icon_60x60.png -small_icon ${RESOURCES_PATH}/icon_30x30.png -name ${APP_NAME} -type Activity -glance_capable -out ${CMAKE_CURRENT_BINARY_DIR} -appid ${APP_ID} -appver ${BUILD_VERSION} -scripts ${SCRIPTS_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Tmp ${OUTPUT_PATH}/Tmp
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_${BUILD_VERSION}.uapp ${OUTPUT_PATH}/
        COMMENT "Merging ${APP_NAME} application"
    )
endfunction()