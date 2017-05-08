# Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Define common versions of Kexi components used to generate KexiVersion.h
# update these version for every release:
set(PROJECT_VERSION_STRING "3.1 Alpha") # Custom name such as "3.1 Alpha"
set(PROJECT_STABLE_VERSION_MAJOR 3) # 3 for 3.x, 4 for 4.x, etc.
set(PROJECT_STABLE_VERSION_MINOR 1) # 0 for 3.0, 1 for 3.1, etc.
set(PROJECT_VERSION_RELEASE 91)     # 90 for Alpha, increase for next test releases, set 0 for first Stable, etc.
set(KEXI_ALPHA 1) # uncomment only for Alpha
#set(KEXI_BETA 1) # uncomment only for Beta
#set(KEXI_RC 1) # uncomment only for RC
set(KEXI_YEAR 2017) # update every year

# -- do not edit below this line --

set(KEXI_CUSTOM_DISTRIBUTION_VERSION "" CACHE STRING
    "Custom name of Kexi version useful to construct co-installabile releases. Any nonempty directory name is accepted. If specified it will be used in KEXI_DISTRIBUTION_VERSION. If not specified, KEXI_DISTRIBUTION_VERSION will be set to PROJECT_STABLE_VERSION_MAJOR.PROJECT_STABLE_VERSION_MINOR.")

if(KEXI_CUSTOM_DISTRIBUTION_VERSION STREQUAL "")
    set(KEXI_DISTRIBUTION_VERSION "${PROJECT_STABLE_VERSION_MAJOR}.${PROJECT_STABLE_VERSION_MINOR}")
else()
    set(KEXI_DISTRIBUTION_VERSION "${KEXI_CUSTOM_DISTRIBUTION_VERSION}")
endif()

# Relative path name useful to construct co-installabile file names and paths
set(KEXI_BASE_PATH "kexi/${KEXI_DISTRIBUTION_VERSION}")

if(NOT DEFINED KEXI_ALPHA AND NOT DEFINED KEXI_BETA AND NOT DEFINED KEXI_RC)
    set(KEXI_STABLE 1)
endif()

# PROJECT_VERSION_MAJOR is the same as PROJECT_STABLE_VERSION_MAJOR but for unstable x.0
# x is decreased by one, e.g. 3.0 Beta is 2.99.
if(NOT DEFINED KEXI_STABLE AND PROJECT_STABLE_VERSION_MINOR EQUAL 0)
    math(EXPR PROJECT_VERSION_MAJOR "${PROJECT_STABLE_VERSION_MAJOR} - 1")
else()
    math(EXPR PROJECT_VERSION_MAJOR ${PROJECT_STABLE_VERSION_MAJOR})
endif()

# PROJECT_VERSION_MINOR is equal to PROJECT_STABLE_VERSION_MINOR for stable releases,
# equal to 99 for x.0 unstable releases (e.g. it's 3.0 Beta has minor version 99),
# and equal to PROJECT_STABLE_VERSION_MINOR-1 for unstable releases other than x.0.
if(DEFINED KEXI_STABLE)
    set(PROJECT_VERSION_MINOR ${PROJECT_STABLE_VERSION_MINOR})
elseif(PROJECT_STABLE_VERSION_MINOR EQUAL 0)
    set(PROJECT_VERSION_MINOR 99)
else()
    math(EXPR PROJECT_VERSION_MINOR "${PROJECT_STABLE_VERSION_MINOR} - 1")
endif()

# PROJECT_STABLE_VERSION_RELEASE is equal to PROJECT_VERSION_RELEASE for stable releases
# and 0 for unstable ones.
if(DEFINED KEXI_STABLE)
    set(PROJECT_STABLE_VERSION_RELEASE ${PROJECT_VERSION_RELEASE})
else()
    set(PROJECT_STABLE_VERSION_RELEASE 0)
endif()

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_RELEASE})

message(STATUS "Kexi version \"${PROJECT_VERSION_STRING}\" (${PROJECT_VERSION}), distribution version \"${KEXI_DISTRIBUTION_VERSION}\"")

# Define the generic version of the Kexi libraries here
# This makes it easy to advance it when the next Kexi release comes.
# 14 was the last GENERIC_PROJECT_LIB_VERSION_MAJOR of the previous Kexi series
# (2.x) so we're starting with 15 in 3.x series.
if(PROJECT_STABLE_VERSION_MAJOR EQUAL 3)
    math(EXPR GENERIC_PROJECT_LIB_VERSION_MAJOR "${PROJECT_STABLE_VERSION_MINOR} + 15")
else()
    # let's make sure we won't forget to update the "15"
    message(FATAL_ERROR "Reminder: please update offset == 15 used to compute GENERIC_PROJECT_LIB_VERSION_MAJOR to something bigger")
endif()
set(GENERIC_PROJECT_LIB_VERSION "${GENERIC_PROJECT_LIB_VERSION_MAJOR}.0.0")
set(GENERIC_PROJECT_LIB_SOVERSION "${GENERIC_PROJECT_LIB_VERSION_MAJOR}")
