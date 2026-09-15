# Sources and include directories of the Buttons LVGL GUI process, the
# counterpart of TouchGFX-GUI/touchgfx.cmake. Buttons draws no text, so it
# has no assets directory.
file(GLOB_RECURSE GUI_APP_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/gui/src/*.cpp
)

set(GUI_APP_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/gui/include
)
