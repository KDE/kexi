# Additional CMake macros
#
# Copyright (C) 2015-2018 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(__kexi_macros)
  return()
endif()
set(__kexi_macros YES)

#unused: include(KDbCreateSharedDataClasses) # from KDb
include(CheckFunctionExists)
include(GenerateExportHeader)
include(GetGitRevisionDescription)
include(MacroBoolTo01)
include(KexiAddSimpleOption)

string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
string(COMPARE EQUAL "${CMAKE_CXX_COMPILER_ID}" "Clang" CMAKE_COMPILER_IS_CLANG)

# Keep apps in the same bin dir so resources that are kept relative to this dir can be found
# without installing.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

macro(ensure_out_of_source_build _extra_message)
    string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" _isBuildInSource)
    if(isBuildInSource)
        message(FATAL_ERROR "Compiling ${PROJECT_NAME} inside the source directory is not possible. "
                            "${_extra_message}")
    endif()
    unset(_isBuildInSource)
endmacro()

# Sets RELEASE_BUILD to TRUE for release builds
macro(detect_release_build)
    if(NOT DEFINED RELEASE_BUILD)
        # estimate mode by CMAKE_BUILD_TYPE content if not set on cmdline
        string(TOLOWER "${CMAKE_BUILD_TYPE}" _CMAKE_BUILD_TYPE_TOLOWER)
        set(_RELEASE_BUILD_TYPES "release" "relwithdebinfo" "minsizerel")
        list(FIND _RELEASE_BUILD_TYPES "${CMAKE_BUILD_TYPE_TOLOWER}" INDEX)
        if (INDEX EQUAL -1)
            set(RELEASE_BUILD FALSE)
        else()
            set(RELEASE_BUILD TRUE)
        endif()
        unset(_RELEASE_BUILD_TYPES)
        unset(_CMAKE_BUILD_TYPE_TOLOWER)
    endif()
endmacro()

set(ICONS_INSTALL_DIR "${DATA_INSTALL_DIR}/${KEXI_BASE_PATH}/icons")

# Fetches git revision and branch from the source dir of the current build if possible.
# Sets ${PROJECT_NAME_UPPER}_GIT_SHA1_STRING and ${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING variables.
# If git information is not available but ${CMAKE_SOURCE_DIR}/GIT_VERSION file exists,
# it is parsed. This file can be created by scripts while preparing tarballs and is
# supposed to contain two lines: hash and branch.
macro(get_git_revision_and_branch)
  set(${PROJECT_NAME_UPPER}_GIT_SHA1_STRING "")
  set(${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING "")
  get_git_head_revision(GIT_REFSPEC ${PROJECT_NAME_UPPER}_GIT_SHA1_STRING)
  get_git_branch(${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING)
  if(NOT ${PROJECT_NAME_UPPER}_GIT_SHA1_STRING OR NOT ${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING)
    if(EXISTS "${CMAKE_SOURCE_DIR}/GIT_VERSION")
      file(READ "${CMAKE_SOURCE_DIR}/GIT_VERSION" _ver)
      string(REGEX REPLACE "\n" ";" _ver "${_ver}")
      list(GET _ver 0 ${PROJECT_NAME_UPPER}_GIT_SHA1_STRING)
      list(GET _ver 1 ${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING)
    endif()
  endif()
  if(${PROJECT_NAME_UPPER}_GIT_SHA1_STRING OR ${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING)
    string(SUBSTRING ${${PROJECT_NAME_UPPER}_GIT_SHA1_STRING} 0 7 ${PROJECT_NAME_UPPER}_GIT_SHA1_STRING)
  else()
    set(${PROJECT_NAME_UPPER}_GIT_SHA1_STRING "")
    set(${PROJECT_NAME_UPPER}_GIT_BRANCH_STRING "")
  endif()
endmacro()

# Adds ${PROJECT_NAME_UPPER}_UNFINISHED option. If it is ON, unfinished features
# (useful for testing but may confuse end-user) are compiled-in.
# This option is OFF by default.
macro(add_unfinished_features_option)
  simple_option(${PROJECT_NAME_UPPER}_UNFINISHED
                "Include unfinished features (useful for testing but may confuse end-user)" OFF)
endmacro()

# Adds commands that generate ${_filename}${KEXI_DISTRIBUTION_VERSION}.pc file
# out of ${_filename}.pc.cmake file and installs the .pc file to ${LIB_INSTALL_DIR}/pkgconfig.
# These commands are not executed for WIN32.
# ${CMAKE_SOURCE_DIR}/${_filename}.pc.cmake should exist.
macro(add_pc_file _filename)
  if (NOT WIN32)
    set(_name ${_filename}${KEXI_DISTRIBUTION_VERSION})
    configure_file(${CMAKE_SOURCE_DIR}/${_filename}.pc.cmake ${CMAKE_BINARY_DIR}/${_name}.pc @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/${_name}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
  endif()
endmacro()

# Sets detailed version information for library co-installability.
# - adds PROJECT_STABLE_VERSION_MAJOR to the lib name
# - sets VERSION to PROJECT_STABLE_VERSION_MAJOR.PROJECT_STABLE_VERSION_MINOR.PROJECT_STABLE_VERSION_RELEASE
# - sets SOVERSION to KEXI_DISTRIBUTION_VERSION
# - sets OUTPUT_NAME to ${_target}${KEXI_DISTRIBUTION_VERSION}
# - sets ${_target_upper}_BASE_NAME variable to the final lib name
# - sets ${_target_upper}_BASE_NAME_LOWER variable to the final lib name, lowercase
# - sets ${_target_upper}_INCLUDE_INSTALL_DIR to include dir for library headers
# - (where _target_upper is uppercase ${_target}
macro(set_coinstallable_lib_version _target)
    set(_name ${_target}${KEXI_DISTRIBUTION_VERSION})
    set(_version "${PROJECT_STABLE_VERSION_MAJOR}.${PROJECT_STABLE_VERSION_MINOR}.${PROJECT_STABLE_VERSION_RELEASE}")
    set(_soversion ${KEXI_DISTRIBUTION_VERSION})
    set_target_properties(${_target}
        PROPERTIES VERSION ${_version}
                   SOVERSION ${_soversion}
                   EXPORT_NAME ${_target}
                   OUTPUT_NAME ${_name}
    )
    string(TOUPPER ${_target} _target_upper)
    string(TOUPPER ${_target_upper}_BASE_NAME _var)
    set(${_var} ${_name})
    string(TOLOWER ${_name} ${_var}_LOWER)
    set(${_target_upper}_INCLUDE_INSTALL_DIR ${INCLUDE_INSTALL_DIR}/${_name})
    unset(_soversion)
    unset(_version)
    unset(_target_upper)
    unset(_var)
endmacro()

# Combines add_library() with set_coinstallable_lib_version()
macro(kexi_add_library)
    set(args ${ARGV})
    list(GET args 0 _target)
    add_library(${args})
    set_coinstallable_lib_version(${_target})
    unset(_target)
endmacro()

# Sets detailed version information for executable co-installability.
# - sets OUTPUT_NAME to ${_target}-${KEXI_DISTRIBUTION_VERSION}
macro(set_coinstallable_executable_version _target)
    set_target_properties(${_target}
        PROPERTIES OUTPUT_NAME ${_target}-${KEXI_DISTRIBUTION_VERSION}
    )
endmacro()

# Combines add_executable() with set_coinstallable_executable_version()
macro(kexi_add_executable)
    set(args ${ARGV})
    list(GET args 0 _target)
    add_executable(${args})
    set_coinstallable_executable_version(${_target})
    unset(_target)
endmacro()

# Adds custom target that updates given file in the current working dir using specified
# command and adds its source files to the project files.
# The target is not executed by default, if dependencies to/from other targets are needed
# they can be set separately using add_dependencies().
# Execution of the command shows status "Updating <filename>".
# Options:
#   TARGET - name of the new custom target
#   FILE - <filename> - name of the file that is updated using the command
#   COMMAND <cmd> [args...] - command to be executed
#   SOURCES [files...] - source files that are used by the command
function(add_update_file_target)
    set(options)
    set(oneValueArgs TARGET FILE)
    set(multiValueArgs COMMAND SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    add_custom_target(${ARG_TARGET}
        COMMAND ${ARG_COMMAND}
        SOURCES ${ARG_SOURCES}
        DEPENDS ${ARG_SOURCES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Updating ${ARG_FILE}"
        VERBATIM
    )
endfunction()

if(WIN32)
    set(_chmod_name attrib)
else()
    set(_chmod_name chmod)
endif()
find_program(kexi_chmod_program ${_chmod_name} DOC "chmod program")
if(kexi_chmod_program)
    message(STATUS "Found program for changing file permissions: ${kexi_chmod_program}")
else()
    message(WARNING "Could not find \"${_chmod_name}\" program for changing file permissions")
endif()

# Sets file or directory read-only for all (non-root) users.
# The _path should exist. If it is not absolute, ${CMAKE_CURRENT_SOURCE_DIR} path is prepended.
function(kexi_set_file_read_only _path)
    if(NOT kexi_chmod_program)
        return()
    endif()
    if(IS_ABSOLUTE ${_path})
        set(_fullpath ${_path})
    else()
        set(_fullpath ${CMAKE_CURRENT_SOURCE_DIR}/${_path})
    endif()
    if(NOT EXISTS ${_fullpath})
        message(FATAL_ERROR "File or directory \"${_fullpath}\" does not exist")
        return()
    endif()
    if(WIN32)
        set(_command "${kexi_chmod_program}" +R "${_fullpath}")
    else()
        set(_command "${kexi_chmod_program}" a-w "${_fullpath}")
    endif()
    execute_process(
        COMMAND ${_command}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _output
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Command failed (${_result}): ${_command}\n\nOutput:\n${_output}")
    endif()
endfunction()

# Adds example KEXI project (.kexi file)
# - sets it read-only in the source directory
# - installs to ${KEXI_EXAMPLES_INSTALL_DIR} as read-only for everyone
macro(kexi_add_example_project _path)
    #DISABLED - source code should not be modified even regarding the read-only flag
    #TODO: port the shell scripts from src/examples to cmake to generate .kexi file(s)
    #      into build dirs and remove the .kexi example files from git. Keep .kexi.sql only in git.
    #kexi_set_file_read_only(${_path})

    install(FILES ${_path} DESTINATION ${KEXI_EXAMPLES_INSTALL_DIR}
            PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
endmacro()
