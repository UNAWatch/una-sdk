# Helper macros to convert spaces into question marks and back again
e := 
sp := $(e) $(e)
qs = $(subst ?,$(sp),$1)
sq = $(subst $(sp),?,$1)

# Get name of this Makefile (avoid getting word 0 and a starting space)
makefile_name := $(wordlist 1,1000,$(MAKEFILE_LIST))

# Get path of this Makefile
makefile_path := $(call qs,$(dir $(call sq,$(abspath $(call sq,$(makefile_name))))))

# Get path where the Application is
application_path := $(call qs,$(abspath $(call sq,$(makefile_path)../..)))

.PHONY: clean assets all

ifneq ($(words $(makefile_path))$(words $(MAKEFILE_LIST)),11)
all clean assets:
$(error Spaces not allowed in path)
else

define replace_car
$(subst $(3),$(2),$(1))
endef

#Change symbol \ to /
SDK := $(call replace_car,$(UNA_SDK),/,\)
#Add path to seach source files in UNA-SDK
VPATH := $(SDK)

#If you want to add a source file from UNA-SDK, just write the path starting with the internal UNA-SDK folder
ADDITIONAL_SOURCES_UNA :=\
Libs/Source/Simulator/Kernel/Mock/Backlight.cpp\
Libs/Source/Simulator/Kernel/Mock/Buzzer.cpp\
Libs/Source/Simulator/Kernel/Mock/FileSystem.cpp\
Libs/Source/Simulator/Kernel/Mock/Vibro.cpp\
Libs/Source/UnaLogger/Logger.cpp\
Libs/Source/Wrappers/StdLibWrappers.c\
ThirdParty/coreJSON/source/core_json.c\
ThirdParty/FitSDKRelease_21.171.00/c/fit.c\
ThirdParty/FitSDKRelease_21.171.00/c/fit_convert.c\
ThirdParty/FitSDKRelease_21.171.00/c/fit_crc.c\
ThirdParty/FitSDKRelease_21.171.00/c/fit_product.c\
Libs/Source/JSON/JsonStreamReader.cpp\
Libs/Source/JSON/JsonStreamWriter.cpp\
Libs/Source/TrackMap/TrackMapBuilder.cpp\
Libs/Source/Kernel/KernelBuilder.cpp\
Libs/Source/FitHelper/FitHelper.cpp\
Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp\
Libs/Source/SensorLayer/SensorConnection.cpp\
Libs/Source/Simulator/App/DualAppComm.cpp\
Libs/Source/Simulator/App/AppCore.cpp\
Libs/Source/Simulator/App/AppMessageCore.cpp\
Libs/Source/Simulator/App/KernelMessageDispatcher.cpp\
Libs/Source/Simulator/App/MessageManager.cpp\
Libs/Source/Simulator/Kernel/Kernel.cpp\
Libs/Source/Simulator/Kernel/Mock/System.cpp\
Libs/Source/Simulator/OS/OS.cpp\
Libs/Source/Simulator/OS/SwTimer.cpp\
Libs/Source/Simulator/OS/OneShotTimer.cpp\
Libs/Source/Simulator/Components/SensorManager.cpp\
Libs/Source/Simulator/Components/SensorDataSample.cpp\
Libs/Source/Simulator/Components/SensorDataQueue.cpp\
Libs/Source/Simulator/Components/SampleRateAdapter.cpp\
Libs/Source/Simulator/Components/InstanceSensorLayer.cpp\
Libs/Source/Simulator/Components/Sensors/Gps/GpsSpeed.cpp\
Libs/Source/Simulator/Components/Sensors/Gps/GpsLocation.cpp\
Libs/Source/Simulator/Components/Sensors/Gps/GpsDistance.cpp\
Libs/Source/Simulator/Components/Sensors/Gps/GpsAltimeter.cpp\
Libs/Source/Simulator/Components/Sensors/HeartRate/SensorHeartRateMetrics.cpp\
Libs/Source/Simulator/Components/Sensors/SensorBatteryLevel.cpp\
Libs/Source/Simulator/Components/Sensors/HeartRate/SensorHeartRate.cpp\
Libs/Source/Simulator/Components/Sensors/SensorPressure.cpp\
Libs/Source/Simulator/Components/Sensors/Imu/ImuWristMotion.cpp\
Libs/Source/Simulator/Components/Sensors/Imu/ImuStepCounter.cpp\
Libs/Source/Simulator/Components/ComponentSimulator.cpp\
Libs/Source/Simulator/Components/Simulator/GpsStepCounterSimulator.cpp\
Libs/Source/Simulator/Components/SensorListener.cpp\
Libs/Source/Simulator/Components/SensorDriver.cpp\
../../Libs/Sources/ActivityWriter.cpp\
../../Libs/Sources/Service.cpp\
ADDITIONAL_SOURCES :=
ADDITIONAL_INCLUDE_PATHS := $(SDK)/Libs/Header\
../../Libs/Header\
$(SDK)/ThirdParty/coreJSON/source/include\
$(SDK)/ThirdParty/FitSDKRelease_21.171.00/c\
ADDITIONAL_LIBRARY_PATHS := 
ADDITIONAL_LIBRARIES :=

export ADDITIONAL_SOURCES ADDITIONAL_INCLUDE_PATHS ADDITIONAL_LIBRARY_PATHS ADDITIONAL_LIBRARIES ADDITIONAL_SOURCES_UNA SDK VPATH

all: $(filter assets,$(MAKECMDGOALS))
all assets: $(filter clean,$(MAKECMDGOALS))
all clean assets:
	@$(MAKE) -r -f una/Makefile -s $(MFLAGS) $@ -C "$(application_path)"
endif

