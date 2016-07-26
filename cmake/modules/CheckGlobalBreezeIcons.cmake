# Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Builds and executes runtime check for existence of Breeze icons
# No variables are set, just fails when icons not found.
# Tries hard, works even if there is no graphical interface available.
#
# Requires KEXI_QTGUI_RUNTIME_AVAILABLE and KEXI_ENABLE_QTGUI_FOR_TESTS to be present

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED Xml)

set(CheckGlobalBreezeIcons_flags "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR}/src/main")
if(KEXI_QTGUI_RUNTIME_AVAILABLE AND KEXI_ENABLE_QTGUI_FOR_TESTS)
    find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED Gui)
    set(CheckGlobalBreezeIcons_libs Qt5::Gui Qt5::Xml)
else()
    set(CheckGlobalBreezeIcons_libs Qt5::Xml)
endif()

try_run(RUN_RESULT COMPILE_OK
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp
    ${CMAKE_SOURCE_DIR}/cmake/modules/CheckGlobalBreezeIcons.cpp
    LINK_LIBRARIES ${CheckGlobalBreezeIcons_libs}
    CMAKE_FLAGS ${CheckGlobalBreezeIcons_flags}
    COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
    RUN_OUTPUT_VARIABLE RUN_OUTPUT)

if(NOT COMPILE_OK)
    message(FATAL_ERROR "${COMPILE_OUTPUT}")
endif()

if(NOT ${RUN_RESULT} EQUAL 0)
    message(FATAL_ERROR "No valid breeze-icons.rcc resource file found. \
The CheckGlobalBreezeIcons.cmake script returned ${RUN_RESULT}.\n \
Result: ${RUN_OUTPUT}")
endif()
