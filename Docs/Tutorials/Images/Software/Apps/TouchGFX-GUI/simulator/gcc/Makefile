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
#Add path seach source files
VPATH := $(SDK)\
		 ../../

#If you want to add a source file from UNA-SDK, just write the path starting with the internal UNA-SDK folder
ADDITIONAL_SOURCES_UNA :=\
../../Libs/Sources/Service.cpp\
Libs/Source/Simulator/Kernel/Mock/Buzzer.cpp\
Libs/Source/Simulator/Kernel/Mock/FileSystem.cpp\
Libs/Source/Simulator/Kernel/Mock/Vibro.cpp\
Libs/Source/Simulator/OS/OS.cpp\
Libs/Source/UnaLogger/Logger.cpp\
Libs/Source/Simulator/Kernel/Mock/Backlight.cpp\
Libs/Source/Wrappers/StdLibWrappers.c\
Libs/Source/Kernel/KernelBuilder.cpp\
Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp\
Libs/Source/Simulator/App/AppComm.cpp\
Libs/Source/Simulator/App/AppCore.cpp\
Libs/Source/Simulator/App/AppMessageCore.cpp\
Libs/Source/Simulator/App/KernelMessageDispatcher.cpp\
Libs/Source/Simulator/App/MessageManager.cpp\
Libs/Source/Simulator/Kernel/Kernel.cpp\
Libs/Source/Simulator/Kernel/Mock/System.cpp
ADDITIONAL_SOURCES :=
ADDITIONAL_INCLUDE_PATHS := $(SDK)/Libs/Header\
../../Libs/Header\
ADDITIONAL_LIBRARY_PATHS := 
ADDITIONAL_LIBRARIES :=

export ADDITIONAL_SOURCES ADDITIONAL_INCLUDE_PATHS ADDITIONAL_LIBRARY_PATHS ADDITIONAL_LIBRARIES ADDITIONAL_SOURCES_UNA SDK VPATH

all: $(filter assets,$(MAKECMDGOALS))
all assets: $(filter clean,$(MAKECMDGOALS))
all clean assets:
	@$(MAKE) -r -f una/Makefile -s $(MFLAGS) $@ -C "$(application_path)"
endif