# UNA SDK Definitions
# Extracted from una-app.cmake

set(UNA_SDK_SOURCES_COMMON
    "${UNA_SDK}/Libs/Source/AppSystem/AtExitImpl.cpp"
    "${UNA_SDK}/Libs/Source/AppSystem/startup_user_app.s"
    "${UNA_SDK}/Libs/Source/AppSystem/system.cpp"
    "${UNA_SDK}/Libs/Source/Kernel/KernelBuilder.cpp"
    "${UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"
)

set(UNA_SDK_SOURCES_SERVICE
    "${UNA_SDK}/Libs/Source/AppSystem/EntryPoint/Service/main.cpp"
    "${UNA_SDK}/Libs/Source/FitHelper/FitHelper.cpp"
    "${UNA_SDK}/Libs/Source/JSON/JsonStreamReader.cpp"
    "${UNA_SDK}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "${UNA_SDK}/Libs/Source/SensorLayer/SensorConnection.cpp"
    "${UNA_SDK}/Libs/Source/TrackMap/TrackMapBuilder.cpp"
    "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit.c"
    "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_convert.c"
    "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_crc.c"
    "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_product.c"
    "${UNA_SDK}/ThirdParty/coreJSON/source/core_json.c"
)

set(UNA_SDK_SOURCES_GUI
    "${UNA_SDK}/Libs/Source/AppSystem/EntryPoint/TouchGFX/main.cpp"
    "${UNA_SDK}/Port/TouchGFX/STM32TouchController.cpp"
    "${UNA_SDK}/Port/TouchGFX/TouchGFXCommandProcessor.cpp"
    "${UNA_SDK}/Port/TouchGFX/TouchGFXGPIO.cpp"
    "${UNA_SDK}/Port/TouchGFX/TouchGFXHAL.cpp"
    "${UNA_SDK}/Port/TouchGFX/generated/OSWrappers.cpp"
    "${UNA_SDK}/Port/TouchGFX/generated/STM32DMA.cpp"
    "${UNA_SDK}/Port/TouchGFX/generated/TouchGFXConfiguration.cpp"
    "${UNA_SDK}/Port/TouchGFX/generated/TouchGFXGeneratedHAL.cpp"
)

set(UNA_SDK_INCLUDE_DIRS_COMMON
    "${UNA_SDK}/Libs/Header"
)

set(UNA_SDK_INCLUDE_DIRS_SERVICE
    "${UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c"
    "${UNA_SDK}/ThirdParty/coreJSON/source/include"
)

set(UNA_SDK_INCLUDE_DIRS_GUI
    "${UNA_SDK}/Port/TouchGFX"
    "${UNA_SDK}/Port/TouchGFX/generated"
)

set(SCRIPTS_PATH "${UNA_SDK}/Utilities/Scripts")