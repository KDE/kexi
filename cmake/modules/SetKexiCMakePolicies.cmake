# Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

cmake_policy(SET CMP0017 NEW)
cmake_policy(SET CMP0048 NEW) # for PROJECT_VERSION
cmake_policy(SET CMP0053 NEW) # TODO remove, temporary fix for a bug in Qt 5.8's Qt5ModuleLocation.cmake
                              # "Simplify variable reference and escape sequence evaluation"

if(POLICY CMP0063) # Honor visibility properties for all target types (since cmake 3.3)
    cmake_policy(SET CMP0063 NEW)
endif()
if(POLICY CMP0071) # Don't warn when combining AUTOMOC with qt5_wrap_ui() or qt5_add_resources() (since cmake 3.10)
    cmake_policy(SET CMP0071 NEW)
endif()
