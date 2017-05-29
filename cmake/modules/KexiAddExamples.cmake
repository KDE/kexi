# Additional CMake macros
#
# Copyright (C) 2015-2017 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(__kexi_add_examples)
  return()
endif()
set(__kexi_add_examples YES)

include(KexiAddSimpleOption)

# Adds BUILD_EXAMPLES option to enable examples. If enabled, build in examples/ subdirectory
# is enabled. If optional argument ARG1 is ON, building examples will be ON by default.
# Otherwise building examples will be OFF. ARG1 is OFF by default.
macro(kexi_add_examples)
  set(_SET ${ARGV0})
  if (NOT "${_SET}" STREQUAL ON)
    set(_SET OFF)
  endif()
  simple_option(BUILD_EXAMPLES "Build example applications" ${_SET})
  if (BUILD_EXAMPLES AND EXISTS ${CMAKE_SOURCE_DIR}/examples)
    add_subdirectory(examples)
  endif()
endmacro()
