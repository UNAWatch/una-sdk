# UNA App CMake Project Configuration

if(NOT DEFINED ENV{UNA_SDK})
    message(FATAL_ERROR "UNA_SDK environment variable must be set for external apps")
endif()
if(NOT DEFINED OUTPUT_PATH)
    set(OUTPUT_PATH ${CMAKE_SOURCE_DIR}/build)
endif()
if(NOT DEFINED RESOURCES_PATH)
    set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
endif()

# Python interpreter for pack/merge scripts.
# Prefer python3 on Windows because the python launcher shim can be broken.
if(NOT DEFINED UNA_PYTHON_EXECUTABLE)
    find_program(UNA_PYTHON_EXECUTABLE NAMES python3 python)
    if(NOT UNA_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "Python interpreter not found. Install python3 or set UNA_PYTHON_EXECUTABLE.")
    endif()
endif()

# Common toolchain setup
set(CMAKE_TOOLCHAIN_FILE "$ENV{UNA_SDK}/cmake/toolchain-arm-none-eabi.cmake")

# Enable assembler language
enable_language(ASM)

# Set standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Add this to your CMakeLists.txt is you want verbose compiler log
# set(CMAKE_VERBOSE_MAKEFILE ON)

# Make the build independent of where the SDK and the app are checked out.
#
# __FILE__ reaches .rodata two ways: assert() (nothing here defines NDEBUG, so
# newlib's __assert_func keeps it) and UnaLogger's __FILENAME__, which trims to a
# basename only at *runtime*. SDK and app sources both compile through absolute
# paths, so the same source built elsewhere produces different bytes.
#
# Configuring -DCMAKE_BUILD_TYPE=Release would define NDEBUG, which takes the
# assert channel and only that one -- __FILENAME__ is gated on LOG_LEVEL. It
# happens to empty a built app of these strings anyway, because assert is today
# the only channel that contributes any: no translation unit that leaves
# LOG_MODULE_PRX at its __FILENAME__ default goes on to call LOG_*. That is a
# coincidence of the current sources, not a property of the flag, so the
# reproducible-build CI job requires the rewritten prefixes to still be present
# rather than trusting either channel to stay non-empty.
#
# -fmacro-prefix-map rewrites __FILE__ only: logged basenames are unchanged and
# debug info keeps real paths, which -ffile-prefix-map would have broken.
#
# Unprobed, unlike -fcyclomatic-complexity below: that one exists only in ST's
# fork, while -fmacro-prefix-map has been mainline GCC since 8.
#
# ABSOLUTE, not REALPATH: the compiler sees the path as CMake spells it
# ("$ENV{UNA_SDK}/Libs/...", see cmake/una-sdk.cmake) and CMake does not resolve
# symlinks, so a REALPATH prefix silently stops matching whenever UNA_SDK is
# reached through one (a symlinked checkout, or macOS /tmp -> /private/tmp).
get_filename_component(UNA_SDK_ABSPATH "$ENV{UNA_SDK}" ABSOLUTE)
add_compile_options(
    $<$<COMPILE_LANGUAGE:C,CXX>:-fmacro-prefix-map=${UNA_SDK_ABSPATH}=/una-sdk>
)

# App sources live in sibling directories above the project dir, so CMake compiles
# them absolutely too, and several of the example apps assert in their own sources.
# Broadest first: where two prefixes both match, GCC applies the one given *last*.
#
# Truthy, not DEFINED: get_filename_component("" ABSOLUTE) returns the app's own
# source dir, so a defined-but-empty LIBS_PATH would emit that as a map *after*
# the one above and, by that same last-wins rule, relabel the app's own sources.
set(_una_app_prefix_maps "")
get_filename_component(_una_abs "${CMAKE_SOURCE_DIR}" ABSOLUTE)
list(APPEND _una_app_prefix_maps "${_una_abs}=/una-app")
if(LIBS_PATH)
    get_filename_component(_una_abs "${LIBS_PATH}" ABSOLUTE)
    list(APPEND _una_app_prefix_maps "${_una_abs}=/una-app-libs")
endif()
if(TOUCHGFX_PATH)
    get_filename_component(_una_abs "${TOUCHGFX_PATH}" ABSOLUTE)
    list(APPEND _una_app_prefix_maps "${_una_abs}=/una-app-gui")
endif()
foreach(_una_map IN LISTS _una_app_prefix_maps)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C,CXX>:-fmacro-prefix-map=${_una_map}>
    )
endforeach()
unset(_una_abs)
unset(_una_map)
unset(_una_app_prefix_maps)

# Common compile options (match CubeIDE exactly)
add_compile_options(
    -mcpu=cortex-m33
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -Os
    -fPIC
    $<$<COMPILE_LANGUAGE:C,CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:C,CXX>:-ffunction-sections>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fdata-sections>
    $<$<COMPILE_LANGUAGE:C,CXX>:-fstack-usage>
    -Wl,--gc-sections 
    -nostartfiles
    -nodefaultlibs
    -nostdlib
    -mthumb
    -ffunction-sections
)

# C++ specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -fno-use-cxa-atexit")

# ASM specific flags
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -x assembler-with-cpp")

# Common linker options
set(UNA_APP_COMMON_LINK_OPTIONS
    -Wl,--gc-sections 
    -nostartfiles
    -nodefaultlibs
    -nostdlib
    -static
    -Wl,--emit-relocs
    -L "$ENV{UNA_SDK}/Libs/Source/AppSystem/linker"
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

    # Set version using una-version.sh script (apps-v* tags in merged una-sdk)
    execute_process(
        COMMAND bash $ENV{UNA_SDK}/Utilities/Scripts/build-cube/una-version.sh ${WORKING_DIR} apps-
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

# -fcyclomatic-complexity exists only in ST's GNU Tools for STM32 fork; mainline
# arm-none-eabi-gcc rejects it outright. Probed rather than assumed so both
# toolchains work unchanged. This runs from the build functions below, not at
# include time, because the languages are not enabled until the app calls
# project(). C and CXX are probed separately so each language is gated by its
# own compiler's answer rather than the other's.
function(una_app_add_metrics_flags TARGET_NAME)
    include(CheckCCompilerFlag)
    include(CheckCXXCompilerFlag)
    check_c_compiler_flag(-fcyclomatic-complexity UNA_HAVE_FCYCLOMATIC_COMPLEXITY_C)
    check_cxx_compiler_flag(-fcyclomatic-complexity UNA_HAVE_FCYCLOMATIC_COMPLEXITY_CXX)
    if(UNA_HAVE_FCYCLOMATIC_COMPLEXITY_C)
        target_compile_options(${TARGET_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:-fcyclomatic-complexity>
        )
    endif()
    if(UNA_HAVE_FCYCLOMATIC_COMPLEXITY_CXX)
        target_compile_options(${TARGET_NAME} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-fcyclomatic-complexity>
        )
    endif()
endfunction()

# Function to build service executable
# Needs:
# - TARGET_NAME - arg
# - UNA_APP_SERVICE_RAM_LENGTH - optional
# - UNA_APP_SERVICE_STACK_SIZE - optional 
# - BUILD_VERSION
# - APP_NAME
# - DEV_ID
# - APP_ID
# - SERVICE_INCLUDE_DIRS
# - SERVICE_SOURCES
function(una_app_build_service TARGET_NAME)
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

    # Print variable values to highlight no hidden dependencies
    message("UNA_APP_SERVICE_STACK_SIZE: ${UNA_APP_SERVICE_STACK_SIZE}")
    message("UNA_APP_SERVICE_RAM_LENGTH: ${UNA_APP_SERVICE_RAM_LENGTH}")
    message("APP_ID: ${APP_ID}")
    message("APP_NAME: ${APP_NAME}")
    message("DEV_ID: ${DEV_ID}")

    add_executable(${TARGET_NAME} ${SERVICE_SOURCES})

    una_app_add_metrics_flags(${TARGET_NAME})

    target_include_directories(${TARGET_NAME} PRIVATE ${SERVICE_INCLUDE_DIRS})

    target_compile_definitions(${TARGET_NAME} PRIVATE
        BUILD_VERSION="${BUILD_VERSION}"
        APP_NAME="${APP_NAME}"
        DEV_ID="${DEV_ID}"
        APP_ID="${APP_ID}"
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        -Wl,--start-group
        $ENV{UNA_SDK}/Libs/Source/AppSystem/Libc++/libstdc++.a
        -Wl,--end-group
    )

    target_link_options(${TARGET_NAME} PRIVATE
        -T "$ENV{UNA_SDK}/Libs/Source/AppSystem/linker/Main/Sections.ld"
        -Wl,--defsym=STACK_SIZE=${UNA_APP_SERVICE_STACK_SIZE}
        -Wl,--defsym=RAM_LENGTH=${UNA_APP_SERVICE_RAM_LENGTH}
        -Wl,-Map=${OUTPUT_PATH}/${TARGET_NAME}.elf.map
        -Wl,-L "$ENV{UNA_SDK}/Libs/Source/AppSystem/Libc++"
        ${UNA_APP_COMMON_LINK_OPTIONS}
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${UNA_PYTHON_EXECUTABLE} $ENV{UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext srv
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Function to build GUI executable
# Needs:
# - TARGET_NAME - arg
# - GUI_SOURCES
# - GUI_INCLUDE_DIRS
# - UNA_APP_SERVICE_RAM_LENGTH - optional
# - UNA_APP_SERVICE_STACK_SIZE - optional 
function(una_app_build_gui TARGET_NAME)
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

    message("UNA_APP_GUI_STACK_SIZE: ${UNA_APP_GUI_STACK_SIZE}")
    message("UNA_APP_GUI_RAM_LENGTH: ${UNA_APP_GUI_RAM_LENGTH}")

    # Compute library directories from TOUCHGFX_LIBS
    set(TOUCHGFX_LIBS_DIRS "")
    foreach(lib IN LISTS TOUCHGFX_LIBS)
    get_filename_component(lib_dir "${lib}" DIRECTORY)
    list(APPEND TOUCHGFX_LIBS_DIRS "-L${lib_dir}")
    endforeach()
    list(REMOVE_DUPLICATES TOUCHGFX_LIBS_DIRS)

    add_executable(${TARGET_NAME} ${GUI_SOURCES})

    una_app_add_metrics_flags(${TARGET_NAME})

    target_include_directories(${TARGET_NAME} PRIVATE ${GUI_INCLUDE_DIRS})

    target_link_libraries(${TARGET_NAME} PRIVATE
        -Wl,--start-group
        -l:libstdc++.a
        ${TOUCHGFX_LIBS}
        -Wl,--end-group
    )

    target_link_options(${TARGET_NAME} PRIVATE
        ${TOUCHGFX_LIBS_DIRS}
        -Wl,-L "$ENV{UNA_SDK}/Libs/Source/AppSystem/Libc++"
        -T "$ENV{UNA_SDK}/Libs/Source/AppSystem/linker/Main/Sections.ld"
        -Wl,--defsym=STACK_SIZE=${UNA_APP_GUI_STACK_SIZE}
        -Wl,--defsym=RAM_LENGTH=${UNA_APP_GUI_RAM_LENGTH}
        -Wl,-Map=${OUTPUT_PATH}/${TARGET_NAME}.elf.map
        ${UNA_APP_COMMON_LINK_OPTIONS}
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${UNA_PYTHON_EXECUTABLE} $ENV{UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext gui
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Read a boolean app option: apply the default when the caller left it unset,
# and reject a value that is neither on nor off.
#
# CMake defines a short list of false values and treats everything else as
# true, so an unrecognised value -- a typo, or a word like "disabled" -- reads
# as ON. These options decide what goes into the packed header, so a value that
# cannot be read as a boolean stops the build rather than picking a side.
# The accepted set is narrower than CMake's: an empty value, NOTFOUND and
# IGNORE all count as false to CMake, but here they mean a caller wrote the
# option and got it wrong, so they are rejected rather than read as off.
function(una_app_bool_option NAME DEFAULT)
    if(NOT DEFINED ${NAME})
        set(${NAME} ${DEFAULT} PARENT_SCOPE)
        return()
    endif()

    # TOUPPER settles the case, so the list is one entry per value, not per
    # spelling: On, on and ON all arrive here as ON.
    string(TOUPPER "${${NAME}}" VALUE)
    set(ACCEPTED ON OFF TRUE FALSE YES NO Y N 1 0)
    if(NOT VALUE IN_LIST ACCEPTED)
        list(JOIN ACCEPTED " " SPELLINGS)
        message(FATAL_ERROR
            "${NAME} is '${${NAME}}', which is not a boolean; accepted in any case: ${SPELLINGS}")
    endif()
endfunction()

# Main function to build a complete watch app
function(una_app_build_app)
    set(OUTPUT_COPY_COMMANDS "")
    foreach(files IN LISTS OUTPUT_PATH)
        list(APPEND OUTPUT_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/*.uapp ${files}/
        )
    endforeach()

    # Final app merging. The merge needs the GUI ELF packed first, so ask whether
    # one was built rather than whether TouchGFX built it: a CustomGUI app sets no
    # TOUCHGFX_PATH, and keying on that left app_merging.py racing the packer.
    # TOUCHGFX_PATH stays for apps calling this before una_app_build_gui(), where
    # the target does not exist yet and DEPENDS resolves at generate time.
    set(APP_DEPENDS ${APP_NAME}Service.elf)
    if(TARGET ${APP_NAME}GUI.elf OR DEFINED TOUCHGFX_PATH)
        list(APPEND APP_DEPENDS ${APP_NAME}GUI.elf)
    endif()
    set(APP_AUTOSTART_FLAG "")
    una_app_bool_option(APP_AUTOSTART Off)
    if(APP_AUTOSTART)
        set(APP_AUTOSTART_FLAG "-autostart")
        message("App autostart is ON")
    else()
        message("App autostart is OFF")
    endif()
    if(NOT DEFINED APP_USER_NAME)
        set(APP_USER_NAME ${APP_NAME})
    endif()

    # The packed header states two independent things: the app's type, and
    # whether it exposes a glance interface (bit 5, 0x20). This flag used to be
    # passed unconditionally, so every image claimed an interface it had no code
    # for, and its two statements contradicted each other.
    #
    # The firmware resolves an app's role from those flags and has to pick an
    # order when they disagree. Under the current order the glance bit outranks
    # the Clockface type, so a watchface was taken for a glance; Activity and
    # Utility are resolved before the bit is consulted and were unaffected.
    #
    # Default follows the type: a Glance app keeps the flag (redundant with its
    # type, but it is the one place the claim is true, and dropping it would
    # change bytes for apps already in the field). Everything else must opt in
    # with APP_GLANCE_INTF, which is what a non-Glance app that really does
    # serve glance data should set.
    set(APP_GLANCE_INTF_FLAG "")
    if(APP_TYPE STREQUAL "Glance")
        una_app_bool_option(APP_GLANCE_INTF On)
    else()
        una_app_bool_option(APP_GLANCE_INTF Off)
    endif()
    if(APP_GLANCE_INTF)
        set(APP_GLANCE_INTF_FLAG "-glance_capable")
        message("App glance interface is ON")
    else()
        message("App glance interface is OFF")
    endif()

    # APP_FILE_NAME pins the .uapp artifact name when the launcher name has to
    # change independently of it: the phone's OTA flow and the CI release zip
    # both key on the artifact name. Left undefined, app_merging.py derives it
    # from APP_USER_NAME as before.
    set(APP_FILE_NAME_ARGS "")
    if(DEFINED APP_FILE_NAME)
        list(APPEND APP_FILE_NAME_ARGS -filename ${APP_FILE_NAME})
        message("App artifact name pinned to: ${APP_FILE_NAME}")
    endif()

    set(APP_ICON_ARGS "")
    una_app_bool_option(APP_USE_ICONS On)

    if(APP_USE_ICONS)
        list(APPEND APP_ICON_ARGS
            -normal_icon ${RESOURCES_PATH}/icon_60x60.png
            -small_icon ${RESOURCES_PATH}/icon_30x30.png
        )
        message("App icons are ON")
    else()
        message("App icons are OFF")
    endif()

    add_custom_target(${APP_NAME}App ALL
        DEPENDS ${APP_DEPENDS}
        COMMAND ${UNA_PYTHON_EXECUTABLE} ${SCRIPTS_PATH}/app_merging/app_merging.py ${APP_AUTOSTART_FLAG} ${APP_ICON_ARGS} -name ${APP_USER_NAME} ${APP_FILE_NAME_ARGS} -type ${APP_TYPE} ${APP_GLANCE_INTF_FLAG} -out ${CMAKE_CURRENT_BINARY_DIR} -appid ${APP_ID} -appver ${BUILD_VERSION} -scripts $ENV{UNA_SDK}/Libs/Source/AppSystem
        ${OUTPUT_COPY_COMMANDS}
        COMMENT "Merging ${APP_NAME} application"
    )
endfunction()
