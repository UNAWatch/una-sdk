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

# Common toolchain setup
set(CMAKE_TOOLCHAIN_FILE $ENV{UNA_SDK}/cmake/toolchain-arm-none-eabi.cmake)

# Enable assembler language
enable_language(ASM)

# Set standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE ON)

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
    -nodefaultlibs
    -nostdlib
    -mthumb
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
        COMMAND python3 $ENV{UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext srv
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
        COMMAND python3 $ENV{UNA_SDK}/Utilities/Scripts/app_packer/app_packer.py -e $<TARGET_FILE:${TARGET_NAME}> -o ${CMAKE_CURRENT_BINARY_DIR}/Tmp -ext gui
        COMMENT "Packing ${TARGET_NAME}"
    )
endfunction()

# Main function to build a complete watch app
function(una_app_build_app)
    set(OUTPUT_COPY_COMMANDS "")
    foreach(files IN LISTS OUTPUT_DIR)
        list(APPEND OUTPUT_COPY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Tmp ${files}/Tmp
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_${BUILD_VERSION}.uapp ${files}/
        )
    endforeach()

    # Final app merging
    set(APP_DEPENDS ${APP_NAME}Service.elf)
    if(DEFINED TOUCHGFX_PATH)
        list(APPEND APP_DEPENDS ${APP_NAME}GUI.elf)
    endif()
    add_custom_target(${APP_NAME}App ALL
        DEPENDS ${APP_DEPENDS}
        COMMAND python3 ${SCRIPTS_PATH}/app_merging/app_merging.py -normal_icon ${RESOURCES_PATH}/icon_60x60.png -small_icon ${RESOURCES_PATH}/icon_30x30.png -name ${APP_NAME} -type ${APP_TYPE} -glance_capable -out ${CMAKE_CURRENT_BINARY_DIR} -appid ${APP_ID} -appver ${BUILD_VERSION} -scripts $ENV{UNA_SDK}/Libs/Source/AppSystem
        ${OUTPUT_COPY_COMMANDS}
        COMMENT "Merging ${APP_NAME} application"
    )
endfunction()
