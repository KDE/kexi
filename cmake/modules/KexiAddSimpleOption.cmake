# Additional CMake macros
#
# Copyright (C) 2015-2017 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(__kexi_add_simple_option)
  return()
endif()
set(__kexi_add_simple_option YES)

include(FeatureSummary)

# Adds a feature info using add_feature_info() with _NAME and _DESCRIPTION.
# If _NAME is equal to _DEFAULT, shows this fact.
macro(add_simple_feature_info _NAME _DESCRIPTION _DEFAULT)
  if("${_DEFAULT}" STREQUAL "${${_NAME}}")
    set(_STATUS " (default value)")
  else()
    set(_STATUS "")
  endif()
  add_feature_info(${_NAME} ${_NAME} ${_DESCRIPTION}${_STATUS})
endmacro()

# Adds a simple option using option() with _NAME and _DESCRIPTION and a feature
# info for it using add_simple_feature_info(). If _NAME is equal to _DEFAULT, shows this fact.
macro(simple_option _NAME _DESCRIPTION _DEFAULT)
  option(${_NAME} ${_DESCRIPTION} ${_DEFAULT})
  add_simple_feature_info(${_NAME} ${_DESCRIPTION} ${_DEFAULT})
endmacro()
