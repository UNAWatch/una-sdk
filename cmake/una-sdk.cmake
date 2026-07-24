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

# Combined service includes for backward compatibility
set(UNA_SDK_INCLUDE_DIRS_SERVICE
    "${UNA_SDK_INCLUDE_DIRS_FIT}"
    "${UNA_SDK_INCLUDE_DIRS_JSON}"
)

set(SCRIPTS_PATH "$ENV{UNA_SDK}/Utilities/Scripts")
