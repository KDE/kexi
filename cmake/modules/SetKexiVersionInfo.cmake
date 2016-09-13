# Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Define common versions of Kexi components used to generate KexiVersion.h
# update these version for every release:
set(KEXI_VERSION_STRING "3.0 Alpha")
set(KEXI_STABLE_VERSION_MAJOR 3) # 3 for 3.x, 4 for 4.x, etc.
set(KEXI_STABLE_VERSION_MINOR 0) # 0 for 3.0, 1 for 3.1, etc.
set(KEXI_VERSION_RELEASE 90)     # 89 for Alpha, increase for next test releases, set 0 for first Stable, etc.
set(KEXI_ALPHA 1) # uncomment only for Alpha
#set(KEXI_BETA 1) # uncomment only for Beta
#set(KEXI_RC 1) # uncomment only for RC
set(KEXI_YEAR 2016) # update every year

# -- do not edit below this line --

if(NOT DEFINED KEXI_ALPHA AND NOT DEFINED KEXI_BETA AND NOT DEFINED KEXI_RC)
    set(KEXI_STABLE 1)
endif()

# KEXI_VERSION_MAJOR is the same as KEXI_STABLE_VERSION_MAJOR but for unstable x.0
# x is decreased by one, e.g. 3.0 Beta is 2.99.
if(NOT DEFINED KEXI_STABLE AND KEXI_STABLE_VERSION_MINOR EQUAL 0)
    math(EXPR KEXI_VERSION_MAJOR "${KEXI_STABLE_VERSION_MAJOR} - 1")
else()
    math(EXPR KEXI_VERSION_MAJOR ${KEXI_STABLE_VERSION_MAJOR})
endif()

# KEXI_VERSION_MINOR is equal to KEXI_STABLE_VERSION_MINOR for stable releases,
# equal to 99 for x.0 unstable releases (e.g. it's 3.0 Beta has minor version 99),
# and equal to KEXI_STABLE_VERSION_MINOR-1 for unstable releases other than x.0.
if(DEFINED KEXI_STABLE)
    set(KEXI_VERSION_MINOR ${KEXI_STABLE_VERSION_MINOR})
elseif(KEXI_STABLE_VERSION_MINOR EQUAL 0)
    set(KEXI_VERSION_MINOR 99)
else()
    math(EXPR KEXI_VERSION_MINOR "${KEXI_STABLE_VERSION_MINOR} - 1")
endif()

# KEXI_STABLE_VERSION_RELEASE is equal to KEXI_VERSION_RELEASE for stable releases
# and 0 for unstable ones.
if(DEFINED KEXI_STABLE)
    set(KEXI_STABLE_VERSION_RELEASE ${KEXI_VERSION_RELEASE})
else()
    set(KEXI_STABLE_VERSION_RELEASE 0)
endif()

set(KEXI_VERSION ${KEXI_VERSION_MAJOR}.${KEXI_VERSION_MINOR}.${KEXI_VERSION_RELEASE})

message(STATUS "Kexi version: ${KEXI_VERSION_STRING} (${KEXI_VERSION})")

# Define the generic version of the Kexi libraries here
# This makes it easy to advance it when the next Kexi release comes.
# 14 was the last GENERIC_KEXI_LIB_VERSION_MAJOR of the previous Kexi series
# (2.x) so we're starting with 15 in 3.x series.
if(KEXI_STABLE_VERSION_MAJOR EQUAL 3)
    math(EXPR GENERIC_KEXI_LIB_VERSION_MAJOR "${KEXI_STABLE_VERSION_MINOR} + 15")
else()
    # let's make sure we won't forget to update the "15"
    message(FATAL_ERROR "Reminder: please update offset == 15 used to compute GENERIC_KEXI_LIB_VERSION_MAJOR to something bigger")
endif()
set(GENERIC_KEXI_LIB_VERSION "${GENERIC_KEXI_LIB_VERSION_MAJOR}.0.0")
set(GENERIC_KEXI_LIB_SOVERSION "${GENERIC_KEXI_LIB_VERSION_MAJOR}")
