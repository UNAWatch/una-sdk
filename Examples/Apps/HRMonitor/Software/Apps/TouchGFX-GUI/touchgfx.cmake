file(GLOB_RECURSE TOUCHGFX_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/gui/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/fonts/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/gui_generated/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/images/src/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/generated/texts/src/*.cpp
)

set(TOUCHGFX_INCLUDE_DIRS
    $ENV{UNA_SDK}/ThirdParty/touchgfx/framework/include
    ${CMAKE_CURRENT_LIST_DIR}/generated/fonts/include
    ${CMAKE_CURRENT_LIST_DIR}/generated/gui_generated/include
    ${CMAKE_CURRENT_LIST_DIR}/generated/images/include
    ${CMAKE_CURRENT_LIST_DIR}/generated/texts/include
    ${CMAKE_CURRENT_LIST_DIR}/generated/videos/include
    ${CMAKE_CURRENT_LIST_DIR}/gui/include
)

set(TOUCHGFX_LIBS
    $ENV{UNA_SDK}/ThirdParty/touchgfx/lib/core/cortex_m33/gcc/libtouchgfx-float-abi-hard.a
)
