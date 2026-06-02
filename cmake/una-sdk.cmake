# UNA SDK Definitions
# Extracted from una-app.cmake

set(UNA_SDK_SOURCES_COMMON
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/AtExitImpl.cpp"
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/startup_user_app.s"
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/system.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Kernel/KernelBuilder.cpp"
    "$ENV{UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Timer/Timer.cpp"
)

set(UNA_SDK_SOURCES_APPSYSTEM
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/EntryPoint/Service/main.cpp"
)

# Selects which vendored FIT SDK release the build links against.
# Apps can override this before include(una-sdk.cmake) to opt in to a newer
# release without affecting the rest of the tree, e.g.
#     set(UNA_FIT_SDK_DIR "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.202.00")
if(NOT DEFINED UNA_FIT_SDK_DIR)
    set(UNA_FIT_SDK_DIR "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.202.00")
endif()

set(UNA_SDK_SOURCES_FIT
    "$ENV{UNA_SDK}/Libs/Source/FitHelper/FitHelper.cpp"
    "$ENV{UNA_SDK}/Libs/Source/FitHelper/FitRecordCadence.cpp"
    "${UNA_FIT_SDK_DIR}/c/fit.c"
    "${UNA_FIT_SDK_DIR}/c/fit_convert.c"
    "${UNA_FIT_SDK_DIR}/c/fit_crc.c"
    "${UNA_FIT_SDK_DIR}/c/fit_product.c"
)

set(UNA_SDK_SOURCES_JSON
    "$ENV{UNA_SDK}/Libs/Source/JSON/JsonStreamReader.cpp"
    "$ENV{UNA_SDK}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "$ENV{UNA_SDK}/ThirdParty/coreJSON/source/core_json.c"
)

set(UNA_SDK_SOURCES_SENSOR
    "$ENV{UNA_SDK}/Libs/Source/SensorLayer/SensorConnection.cpp"
)

set(UNA_SDK_SOURCES_TRACKMAP
    "$ENV{UNA_SDK}/Libs/Source/TrackMap/TrackMapBuilder.cpp"
)

# Combined service sources for backward compatibility
set(UNA_SDK_SOURCES_SERVICE
    "${UNA_SDK_SOURCES_APPSYSTEM}"
    "${UNA_SDK_SOURCES_FIT}"
    "${UNA_SDK_SOURCES_JSON}"
    "${UNA_SDK_SOURCES_SENSOR}"
    "${UNA_SDK_SOURCES_TRACKMAP}"
)

set(UNA_SDK_SOURCES_GUI
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/EntryPoint/TouchGFX/main.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/STM32TouchController.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/TouchGFXGPIO.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/generated/STM32DMA.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/generated/TouchGFXConfiguration.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/generated/TouchGFXGeneratedHAL.cpp"
)

set(UNA_SDK_INCLUDE_DIRS_COMMON
    "$ENV{UNA_SDK}/Libs/Header"
)

set(UNA_SDK_INCLUDE_DIRS_FIT
    "${UNA_FIT_SDK_DIR}/c"
)

set(UNA_SDK_INCLUDE_DIRS_JSON
    "$ENV{UNA_SDK}/ThirdParty/coreJSON/source/include"
)

set(UNA_SDK_INCLUDE_DIRS_GUI
    "$ENV{UNA_SDK}/Libs/Header/SDK/Port/TouchGFX"
    "$ENV{UNA_SDK}/Libs/Header/SDK/Port/TouchGFX/generated"
)

# ---------------------------------------------------------------------------
# LVGL v9 GUI wiring (kernel GUI migration — see Docs/LVGL-Migration/Design.md)
#
# These variables are PARALLEL to UNA_SDK_SOURCES_GUI / UNA_SDK_INCLUDE_DIRS_GUI
# (the TouchGFX wiring above). The TouchGFX variables are intentionally left
# intact so the out-of-scope SDK example apps keep building unchanged. A build
# selects ONE GUI backend by referencing either the *_GUI or the *_GUI_LVGL
# variables — never both (they provide competing GUI entry points / main.cpp).
#
# LVGL itself is expected as a git submodule at the REPO-ROOT ThirdParty/lvgl
# (same prefix the firmware already uses for ST/FatFs/BlueNRG via the CubeIDE
# project's ../../../ThirdParty). This is distinct from SDK/ThirdParty. The
# `git submodule add` step is MANUAL — see Docs/LVGL-Migration/Build-Changes.md.
#
# UNA_LVGL_DIR may be overridden by the caller before include(una-sdk.cmake)
# to point at a non-default LVGL checkout.
if(NOT DEFINED UNA_LVGL_DIR)
    # Repo root = two levels up from this file's SDK/cmake/ directory.
    get_filename_component(_UNA_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    set(UNA_LVGL_DIR "${_UNA_REPO_ROOT}/ThirdParty/lvgl")
endif()

# LVGL core C sources. Globbed because the upstream v9 source tree is large and
# churns between releases; examples/demos/tests are excluded (not built here).
# NOTE: a CMake re-configure is required after the submodule is first added or
# updated so the glob picks up the new files.
file(GLOB_RECURSE UNA_LVGL_CORE_SOURCES
    "${UNA_LVGL_DIR}/src/*.c"
)
list(FILTER UNA_LVGL_CORE_SOURCES EXCLUDE REGEX "/(examples|demos|tests)/")

set(UNA_SDK_SOURCES_GUI_LVGL
    # GUI entry point (replaces the TouchGFX EntryPoint/TouchGFX/main.cpp)
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/EntryPoint/LVGL/main.cpp"
    # SDK LVGL port layer (init, display flush, keypad indev, lifecycle)
    "$ENV{UNA_SDK}/Libs/Source/Port/LVGL/app_lvgl.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/LVGL/lv_port_disp.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/LVGL/lv_port_indev.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/LVGL/lv_port_lifecycle.cpp"
    # LVGL upstream core (submodule glob)
    "${UNA_LVGL_CORE_SOURCES}"
)

set(UNA_SDK_INCLUDE_DIRS_GUI_LVGL
    # SDK LVGL port headers (app_lvgl.h, lv_port_disp.h, lv_port_indev.h,
    # lv_port_lifecycle.h, lv_conf.h)
    "$ENV{UNA_SDK}/Libs/Header/SDK/Port/LVGL"
    # LVGL upstream headers
    "${UNA_LVGL_DIR}"
    "${UNA_LVGL_DIR}/src"
)

# LV_CONF_PATH points LVGL at the canonical kernel lv_conf.h. The contract
# keeps the canonical copy in the SDK port dir; the kernel GUI tree's
# Software/App/LVGL-GUI/lv_conf.h is the editor/IntelliSense mirror.
# Consumers add this to their target_compile_definitions, e.g.
#   target_compile_definitions(<tgt> PRIVATE LV_CONF_PATH="${UNA_LV_CONF_PATH}")
set(UNA_LV_CONF_PATH "$ENV{UNA_SDK}/Libs/Header/SDK/Port/LVGL/lv_conf.h")

# Combined service includes for backward compatibility
set(UNA_SDK_INCLUDE_DIRS_SERVICE
    "${UNA_SDK_INCLUDE_DIRS_FIT}"
    "${UNA_SDK_INCLUDE_DIRS_JSON}"
)

set(SCRIPTS_PATH "$ENV{UNA_SDK}/Utilities/Scripts")
