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

set(UNA_SDK_SOURCES_CALIBRATION
    "$ENV{UNA_SDK}/Libs/Source/Calibration/OutdoorStrideCalibrator.cpp"
)

# Combined service sources for backward compatibility
set(UNA_SDK_SOURCES_SERVICE
    "${UNA_SDK_SOURCES_APPSYSTEM}"
    "${UNA_SDK_SOURCES_FIT}"
    "${UNA_SDK_SOURCES_JSON}"
    "${UNA_SDK_SOURCES_SENSOR}"
    "${UNA_SDK_SOURCES_TRACKMAP}"
    "${UNA_SDK_SOURCES_CALIBRATION}"
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

# Combined service includes for backward compatibility
set(UNA_SDK_INCLUDE_DIRS_SERVICE
    "${UNA_SDK_INCLUDE_DIRS_FIT}"
    "${UNA_SDK_INCLUDE_DIRS_JSON}"
)

set(SCRIPTS_PATH "$ENV{UNA_SDK}/Utilities/Scripts")
