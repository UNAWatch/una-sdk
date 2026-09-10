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

# Native FIT-format encoder (SDK::Fit). No external dependency.
set(UNA_SDK_SOURCES_FIT
    "$ENV{UNA_SDK}/Libs/Source/Fit/FitCrc.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Fit/FitWriter.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Fit/FitRecordCadence.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Fit/RecordingMarker.cpp"
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

# Variant-alias config reader (SDK::Variant). Needs UNA_SDK_SOURCES_JSON in
# the same link (the GUI process must add both to read the config directly).
set(UNA_SDK_SOURCES_VARIANT
    "$ENV{UNA_SDK}/Libs/Source/Variant/VariantConfig.cpp"
)

# Developer-declared configuration fields (SDK::AppConfig). Needs
# UNA_SDK_SOURCES_JSON and UNA_SDK_INCLUDE_DIRS_JSON in the same link; both are
# already in the service lists, so a GUI process that reads the configuration
# itself must add all three. See Docs/app-config-fields.md.
set(UNA_SDK_SOURCES_APPCONFIG
    "$ENV{UNA_SDK}/Libs/Source/AppConfig/AppConfig.cpp"
)

set(UNA_SDK_SOURCES_CALIBRATION
    "$ENV{UNA_SDK}/Libs/Source/Calibration/OutdoorStrideCalibrator.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Calibration/StrideLut.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Calibration/CadenceStrideModel.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Calibration/TreadmillSpeedEstimator.cpp"
)

# Combined service sources for backward compatibility
set(UNA_SDK_SOURCES_SERVICE
    "${UNA_SDK_SOURCES_APPSYSTEM}"
    "${UNA_SDK_SOURCES_FIT}"
    "${UNA_SDK_SOURCES_JSON}"
    "${UNA_SDK_SOURCES_SENSOR}"
    "${UNA_SDK_SOURCES_TRACKMAP}"
    "${UNA_SDK_SOURCES_CALIBRATION}"
    "${UNA_SDK_SOURCES_VARIANT}"
    "${UNA_SDK_SOURCES_APPCONFIG}"
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

# SDK::Fit headers live under Libs/Header (UNA_SDK_INCLUDE_DIRS_COMMON);
# no separate FIT include directory is required.
set(UNA_SDK_INCLUDE_DIRS_FIT)

set(UNA_SDK_INCLUDE_DIRS_JSON
    "$ENV{UNA_SDK}/ThirdParty/coreJSON/source/include"
)

set(UNA_SDK_INCLUDE_DIRS_GUI
    "$ENV{UNA_SDK}/Libs/Header/SDK/Port/TouchGFX"
    "$ENV{UNA_SDK}/Libs/Header/SDK/Port/TouchGFX/generated"
)

# ---------------------------------------------------------------------------
# GUI process built on LVGL (ThirdParty/lvgl submodule) instead of TouchGFX.
#
# An app selects this by linking UNA_SDK_SOURCES_GUI_LVGL in place of
# UNA_SDK_SOURCES_GUI, adding UNA_SDK_INCLUDE_DIRS_GUI_LVGL to its include
# dirs and UNA_SDK_DEFINES_GUI_LVGL to GUI_COMPILE_DEFINITIONS (see
# una_app_build_gui). The message pump shared with the TouchGFX port,
# TouchGFXCommandProcessor.cpp, has no TouchGFX dependency.
#
# LVGL reads its configuration from the file named by LV_CONF_PATH. Set
# UNA_LVGL_CONF before including this file to use an app-specific lv_conf.h;
# the default is the SDK's.
# ---------------------------------------------------------------------------
set(UNA_SDK_LVGL_PATH "$ENV{UNA_SDK}/ThirdParty/lvgl")

if(NOT DEFINED UNA_LVGL_CONF)
    set(UNA_LVGL_CONF "$ENV{UNA_SDK}/Libs/Header/SDK/Port/LVGL/lv_conf.h")
endif()

# Every LVGL C source is compiled; files for disabled features and other
# platforms reduce to empty translation units through lv_conf.h.
file(GLOB_RECURSE UNA_SDK_LVGL_SOURCES CONFIGURE_DEPENDS
    "${UNA_SDK_LVGL_PATH}/src/*.c"
)

set(UNA_SDK_SOURCES_GUI_LVGL
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/EntryPoint/LVGL/main.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Port/LVGL/LvglPort.cpp"
    ${UNA_SDK_LVGL_SOURCES}
)

set(UNA_SDK_INCLUDE_DIRS_GUI_LVGL
    "${UNA_SDK_LVGL_PATH}"
)

# LV_LVGL_H_INCLUDE_SIMPLE makes the C files lv_font_conv and LVGLImage.py emit
# include "lvgl.h" (on the include path above) rather than "lvgl/lvgl.h".
set(UNA_SDK_DEFINES_GUI_LVGL
    "LV_CONF_PATH=\"${UNA_LVGL_CONF}\""
    "LV_LVGL_H_INCLUDE_SIMPLE"
)

# Combined service includes for backward compatibility
set(UNA_SDK_INCLUDE_DIRS_SERVICE
    "${UNA_SDK_INCLUDE_DIRS_FIT}"
    "${UNA_SDK_INCLUDE_DIRS_JSON}"
)

set(SCRIPTS_PATH "$ENV{UNA_SDK}/Utilities/Scripts")
