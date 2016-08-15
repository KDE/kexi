# Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

cmake_policy(SET CMP0002 OLD)
cmake_policy(SET CMP0017 NEW)
cmake_policy(SET CMP0022 OLD)
cmake_policy(SET CMP0048 NEW) # for PROJECT_VERSION

if(POLICY CMP0059) # Don’t treat DEFINITIONS as a built-in directory property.
    cmake_policy(SET CMP0059 OLD)
endif()
#if(POLICY CMP0063) # Honor visibility properties for all target types (since cmake 3.3)
    cmake_policy(SET CMP0063 NEW)
#endif()
