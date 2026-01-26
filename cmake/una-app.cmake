# UNA App CMake Project Configuration
# This file contains common CMake logic extracted from app CMakeLists.txt
# to enable both relative (within SDK) and environment-variable based (external) SDK referencing

# Determine SDK path - check if we're within SDK or external
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../tools/una.py")
    # We're within the SDK repository
    set(UNA_SDK "${CMAKE_CURRENT_LIST_DIR}/..")
    message("Building within SDK repository: ${UNA_SDK}")
else()
    # External app - require UNA_SDK environment variable
    if(NOT DEFINED ENV{UNA_SDK})
        message(FATAL_ERROR "UNA_SDK environment variable must be set for external apps")
    endif()
    set(UNA_SDK "$ENV{UNA_SDK}")
    message("Using external SDK: ${UNA_SDK}")
endif()

include("${UNA_SDK}/cmake/una-sdk.cmake")

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
set(UNA_APP_COMMON_LINK_OPTIONS
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

# Function to set up build version
function(una_app_setup_version BUILD_VERSION_OUT WORKING_DIR)
    if(DEFINED BUILD_VERSION)
        set(${BUILD_VERSION_OUT} "${BUILD_VERSION}" PARENT_SCOPE)
        message("External BUILD_VERSION: ${BUILD_VERSION}")
        return()
    endif()

    # Set version using una-version.sh script
    execute_process(
        COMMAND bash $ENV{UNA_SDK}/Utilities/Scripts/build-cube/una-version.sh
        WORKING_DIRECTORY ${WORKING_DIR}
        OUTPUT_VARIABLE SCRIPT_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "BUILD_VERSION=(.+)$" _ "${SCRIPT_OUTPUT}")
    if(CMAKE_MATCH_1)
        set(BUILD_VERSION "${CMAKE_MATCH_1}")
        set(${BUILD_VERSION_OUT} "${BUILD_VERSION}" PARENT_SCOPE)
        message("Detected BUILD_VERSION: ${BUILD_VERSION}")
        return()
    endif()

    # Fallback
    set(BUILD_VERSION "1.0.0")  # Default fallback

    execute_process(COMMAND git rev-parse --git-dir OUTPUT_VARIABLE GIT_DIR ERROR_QUIET WORKING_DIRECTORY ${WORKING_DIR})
    if(GIT_DIR)
        execute_process(COMMAND git status --porcelain OUTPUT_VARIABLE GIT_STATUS WORKING_DIRECTORY ${WORKING_DIR})
        if(GIT_STATUS)
            set(BUILD_VERSION "1.0.0-dirty")
        endif()
    endif()

    set(${BUILD_VERSION_OUT} "${BUILD_VERSION}" PARENT_SCOPE)
    message("Fallback BUILD_VERSION: ${BUILD_VERSION}")
endfunction()

# Function to build service executable
function(una_app_build_service TARGET_NAME LIBS_PATH TOUCHGFX_GUI_PATH)
    include("${LIBS_PATH}/libs.cmake")

    if(NOT DEFINED UNA_APP_SERVICE_STACK_SIZE)
        if(DEFINED SERVICE_STACK_SIZE)
            set(UNA_APP_SERVICE_STACK_SIZE "${SERVICE_STACK_SIZE}")
        else()
            set(UNA_APP_SERVICE_STACK_SIZE "10*1024")
        endif()
    endif()
    if(NOT DEFINED UNA_APP_SERVICE_RAM_LENGTH)
        if(DEFINED SERVICE_RAM_LENGTH)
            set(UNA_APP_SERVICE_RAM_LENGTH "${SERVICE_RAM_LENGTH}")
        else()
            set(UNA_APP_SERVICE_RAM_LENGTH "500K")
        endif()
    endif()

    # Service sources - app-specific + common SDK sources
    set(SERVICE_SOURCES
        ${LIBS_SOURCES}
        "${CMAKE_CURRENT_SOURCE_DIR}/syscalls.cpp"
        ${UNA_SDK_SOURCES_COMMON}
        ${UNA_SDK_SOURCES_SERVICE}
    )

    add_executable(${TARGET_NAME} ${SERVICE_SOURCES})

    set(SERVICE_INCLUDE_DIRS
        ${LIBS_INCLUDE_DIRS}
        ${UNA_SDK_INCLUDE_DIRS_COMMON}
        ${UNA_SDK_INCLUDE_DIRS_SERVICE}
    )

    if(EXISTS "${TOUCHGFX_GUI_PATH}/gui/include")
        list(APPEND SERVICE_INCLUDE_DIRS "${TOUCHGFX_GUI_PATH}/gui/include")
    endif()

    target_include_directories(${TARGET_NAME} PRIVATE ${SERVICE_INCLUDE_DIRS})

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
        -T "${UNA_SDK}/Utilities/Scripts/linker/Main/Sections.ld"
        -Wl,--defsym=STACK_SIZE=${UNA_APP_SERVICE_STACK_SIZE}
        -Wl,--defsym=RAM_LENGTH=${UNA_APP_SERVICE_RAM_LENGTH}
        -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.elf.map
        ${UNA_APP_COMMON_LINK_OPTIONS}
        -L${UNA_SDK}/libc++
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND python3 ${UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext srv
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Function to build GUI executable
function(una_app_build_gui TARGET_NAME LIBS_PATH TOUCHGFX_GUI_PATH)
    include("${TOUCHGFX_GUI_PATH}/touchgfx.cmake")

    if(NOT DEFINED UNA_APP_GUI_STACK_SIZE)
        if(DEFINED GUI_STACK_SIZE)
            set(UNA_APP_GUI_STACK_SIZE "${GUI_STACK_SIZE}")
        else()
            set(UNA_APP_GUI_STACK_SIZE "10*1024")
        endif()
    endif()
    if(NOT DEFINED UNA_APP_GUI_RAM_LENGTH)
        if(DEFINED GUI_RAM_LENGTH)
            set(UNA_APP_GUI_RAM_LENGTH "${GUI_RAM_LENGTH}")
        else()
            set(UNA_APP_GUI_RAM_LENGTH "600K")
        endif()
    endif()

    set(GUI_SOURCES
        ${TOUCHGFX_SOURCES}
        "${CMAKE_CURRENT_SOURCE_DIR}/PaintImpl.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/syscalls.cpp"
        ${UNA_SDK_SOURCES_COMMON}
        ${UNA_SDK_SOURCES_GUI}
    )

    add_executable(${TARGET_NAME} ${GUI_SOURCES})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${LIBS_INCLUDE_DIRS}
        ${UNA_SDK_INCLUDE_DIRS_COMMON}
        ${UNA_SDK_INCLUDE_DIRS_GUI}
        ${TOUCHGFX_INCLUDE_DIRS}
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        -Wl,--start-group
        ${UNA_SDK}/libc++/libstdc++.a
        ${TOUCHGFX_GUI_PATH}/touchgfx/lib/core/cortex_m33/gcc/libtouchgfx-float-abi-hard.a
        -Wl,--end-group
    )

    target_link_options(${TARGET_NAME} PRIVATE
        -L${TOUCHGFX_GUI_PATH}/touchgfx/lib/core/cortex_m33/gcc
        -T "${UNA_SDK}/Utilities/Scripts/linker/Main/Sections.ld"
        -Wl,--defsym=STACK_SIZE=${UNA_APP_GUI_STACK_SIZE}
        -Wl,--defsym=RAM_LENGTH=${UNA_APP_GUI_RAM_LENGTH}
        -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.elf.map
        ${UNA_APP_COMMON_LINK_OPTIONS}
        -L${UNA_SDK}/libc++
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND python3 ${UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext gui
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Main function to build a complete watch app
function(una_app_build_app)
    if(DEFINED APP_PATH)
        set(APP_ROOT "${APP_PATH}")
    else()
        get_filename_component(LEGACY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
        if(EXISTS "${LEGACY_ROOT}/Software/Libs")
            set(APP_ROOT "${LEGACY_ROOT}")
        else()
            set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
        endif()
    endif()

    if(NOT IS_ABSOLUTE "${APP_ROOT}")
        get_filename_component(APP_ROOT "${APP_ROOT}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # Set up paths relative to app root unless overridden
    if(NOT DEFINED LIBS_PATH)
        if(EXISTS "${APP_ROOT}/Software/Libs")
            set(LIBS_PATH "${APP_ROOT}/Software/Libs")
        else()
            set(LIBS_PATH "${APP_ROOT}/Libs")
        endif()
    endif()
    if(NOT DEFINED TOUCHGFX_GUI_PATH)
        if(EXISTS "${APP_ROOT}/Software/Apps/TouchGFX-GUI")
            set(TOUCHGFX_GUI_PATH "${APP_ROOT}/Software/Apps/TouchGFX-GUI")
        else()
            set(TOUCHGFX_GUI_PATH "${APP_ROOT}/TouchGFX-GUI")
        endif()
    endif()
    if(NOT DEFINED OUTPUT_PATH)
        set(OUTPUT_PATH "${APP_ROOT}/Output")
    endif()
    if(NOT DEFINED RESOURCES_PATH)
        set(RESOURCES_PATH "${APP_ROOT}/Resources")
    endif()

    set(SCRIPTS_PATH "${UNA_SDK}/Utilities/Scripts")

    # Set up version
    una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

    # Make BUILD_VERSION available globally
    set(BUILD_VERSION "${BUILD_VERSION}" PARENT_SCOPE)

    # Build service and GUI
    una_app_build_service(${APP_NAME}Service.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})

    set(OUTPUT_PATHS "")
    foreach(path IN LISTS OUTPUT_PATH)
        if(IS_ABSOLUTE "${path}")
            list(APPEND OUTPUT_PATHS "${path}")
        else()
            list(APPEND OUTPUT_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/${path}")
        endif()
    endforeach()

    if(DEFINED APP_TYPE AND APP_TYPE STREQUAL "service-only")
        foreach(out_path IN LISTS OUTPUT_PATHS)
            add_custom_command(TARGET ${APP_NAME}Service.elf POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Tmp ${out_path}/Tmp
                COMMENT "Copying service output"
            )
        endforeach()
        return()
    endif()

    una_app_build_gui(${APP_NAME}GUI.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})

    set(OUTPUT_COPY_COMMANDS "")
    foreach(out_path IN LISTS OUTPUT_PATHS)
        list(APPEND OUTPUT_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Tmp ${out_path}/Tmp
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_${BUILD_VERSION}.uapp ${out_path}/
        )
    endforeach()

    # Final app merging
    add_custom_target(${APP_NAME}App ALL
        DEPENDS ${APP_NAME}Service.elf ${APP_NAME}GUI.elf
        COMMAND python3 ${SCRIPTS_PATH}/app_merging/app_merging.py -normal_icon ${RESOURCES_PATH}/icon_60x60.png -small_icon ${RESOURCES_PATH}/icon_30x30.png -name ${APP_NAME} -type Activity -glance_capable -out ${CMAKE_CURRENT_BINARY_DIR} -appid ${APP_ID} -appver ${BUILD_VERSION} -scripts ${SCRIPTS_PATH}
        ${OUTPUT_COPY_COMMANDS}
        COMMENT "Merging ${APP_NAME} application"
    )
endfunction()
