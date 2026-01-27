# UNA SDK Definitions
# Extracted from una-app.cmake

set(UNA_SDK_SOURCES_COMMON
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/AtExitImpl.cpp"
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/startup_user_app.s"
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/system.cpp"
    "$ENV{UNA_SDK}/Libs/Source/Kernel/KernelBuilder.cpp"
    "$ENV{UNA_SDK}/Libs/Source/UnaLogger/Logger.cpp"
)

set(UNA_SDK_SOURCES_SERVICE
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/EntryPoint/Service/main.cpp"
    "$ENV{UNA_SDK}/Libs/Source/FitHelper/FitHelper.cpp"
    "$ENV{UNA_SDK}/Libs/Source/JSON/JsonStreamReader.cpp"
    "$ENV{UNA_SDK}/Libs/Source/JSON/JsonStreamWriter.cpp"
    "$ENV{UNA_SDK}/Libs/Source/SensorLayer/SensorConnection.cpp"
    "$ENV{UNA_SDK}/Libs/Source/TrackMap/TrackMapBuilder.cpp"
    "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit.c"
    "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_convert.c"
    "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_crc.c"
    "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c/fit_product.c"
    "$ENV{UNA_SDK}/ThirdParty/coreJSON/source/core_json.c"
)

set(UNA_SDK_SOURCES_GUI
    "$ENV{UNA_SDK}/Libs/Source/AppSystem/EntryPoint/TouchGFX/main.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/STM32TouchController.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/TouchGFXCommandProcessor.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/TouchGFXGPIO.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/TouchGFXHAL.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/generated/OSWrappers.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/generated/STM32DMA.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/generated/TouchGFXConfiguration.cpp"
    "$ENV{UNA_SDK}/Port/TouchGFX/generated/TouchGFXGeneratedHAL.cpp"
)

set(UNA_SDK_INCLUDE_DIRS_COMMON
    "$ENV{UNA_SDK}/Libs/Header"
)

set(UNA_SDK_INCLUDE_DIRS_SERVICE
    "$ENV{UNA_SDK}/ThirdParty/FitSDKRelease_21.171.00/c"
    "$ENV{UNA_SDK}/ThirdParty/coreJSON/source/include"
)

set(UNA_SDK_INCLUDE_DIRS_GUI
    "$ENV{UNA_SDK}/Port/TouchGFX"
    "$ENV{UNA_SDK}/Port/TouchGFX/generated"
)

set(SCRIPTS_PATH "$ENV{UNA_SDK}/Utilities/Scripts")
