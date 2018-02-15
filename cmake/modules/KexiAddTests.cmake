# Additional CMake macros
#
# Copyright (C) 2015-2017 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Note: the file must be included before KDEInstallDirs or add_tests() won't fully work

if(__kexi_add_tests)
  return()
endif()
set(__kexi_add_tests YES)

include(KexiAddSimpleOption)
include(MacroBoolTo01)

# Adds BUILD_TESTING option to enable all kinds of tests. If enabled, build in autotests/
# and tests/ subdirectory is enabled. If optional argument ARG1 is ON, building tests will
# be ON by default. Otherwise building tests will be OFF. ARG1 is OFF by default.
# If tests are OFF, BUILD_COVERAGE is set to OFF.
# If tests are on BUILD_TESTING macro is defined.
macro(kexi_add_tests)
  if(KDE_INSTALL_TARGETS_DEFAULT_ARGS)
      message(FATAL_ERROR "Include before KDEInstallDirs!")
  endif()
  if (NOT "${ARG1}" STREQUAL "ON")
    set(_SET OFF)
  endif()
  simple_option(BUILD_TESTING "Build tests" ${_SET}) # override default from CTest.cmake
  if(BUILD_TESTING)
    add_definitions(-DBUILD_TESTING)
    include(CTest)
  else()
    set(BUILD_COVERAGE OFF)
    simple_option(BUILD_COVERAGE "Build test coverage (disabled because BUILD_TESTING is OFF)" OFF)
  endif()

  # only with COMPILING_TESTS definition will all the FOO_TEST_EXPORT macros do something
  # TODO: check if this can be moved to only those places which make use of it,
  # to reduce global compiler definitions that would trigger a recompile of
  # everything on a change (like adding/removing tests to/from the build)
  macro_bool_to_01(BUILD_TESTING COMPILING_TESTS)
endmacro()
