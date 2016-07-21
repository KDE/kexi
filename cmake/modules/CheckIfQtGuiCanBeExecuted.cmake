# Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Global variables
# KEXI_QTGUI_RUNTIME_AVAILABLE - TRUE if Qt GUI can be used at runtime for configuration and test apps
#
# CMake options
# KEXI_ENABLE_QTGUI_FOR_TESTS - TRUE if configuration or test apps that require Qt GUI
#                               should be executed; by default equal to ${KEXI_QTGUI_RUNTIME_AVAILABLE}

if(NOT DEFINED KEXI_QTGUI_RUNTIME_AVAILABLE)
    set(KEXI_QTGUI_RUNTIME_AVAILABLE TRUE CACHE BOOL "TRUE if Qt GUI can be used at runtime for configuration and test apps")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
        execute_process(COMMAND xset q # check presence of X11; TODO support Wayland, etc.
            RESULT_VARIABLE XSET_RESULT
        )
        if(NOT ${XSET_RESULT} EQUAL 0)
            set(KEXI_QTGUI_RUNTIME_AVAILABLE FALSE)
        endif()
    endif()
    mark_as_advanced(KEXI_QTGUI_RUNTIME_AVAILABLE)
endif()

if(NOT KEXI_QTGUI_RUNTIME_AVAILABLE)
    message(STATUS "Note: Graphical interface is not available so configuration and test apps that require Qt GUI won't be executed")
endif()

option(KEXI_ENABLE_QTGUI_FOR_TESTS "Enable using GUI for running tests or configuration programs. Switch it off if your build environment."
       ${KEXI_QTGUI_RUNTIME_AVAILABLE})
