# Generates dependency graphs for current project
#
# Copyright (C) 2017 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(TARGET generate_dependency_graph)
  return()
endif()

find_package(Doxygen)

if(NOT DOXYGEN_DOT_EXECUTABLE)
    message(STATUS "Graphviz dot tool not found, won't generate dependency graphs")
    return()
endif()

set(_graph_dir ${CMAKE_BINARY_DIR}/dependencies)
set(_dot_file ${_graph_dir}/graph.dot)
set(_image_file "dependency-graph-${CMAKE_PROJECT_NAME}.png")

simple_option(${PROJECT_NAME_UPPER}_DEPENDENCY_GRAPH_INCLUDE_KEXI_FRAMEWORKS "Include Kexi frameworks in the dependency graph" ON)
simple_option(${PROJECT_NAME_UPPER}_DEPENDENCY_GRAPH_INCLUDE_ALL_LIBS "Include all libs in the dependency graph" OFF)

configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeGraphVizOptions.cmake.in
               ${CMAKE_BINARY_DIR}/CMakeGraphVizOptions.cmake)

add_custom_target(generate_dependency_graph)

add_custom_command(
    TARGET generate_dependency_graph POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_graph_dir}
    COMMAND cmake --graphviz=${_dot_file} .
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating dependency graph ${_graph_dir}/${_dot_file} for "
            "${CMAKE_PROJECT_NAME} project"
)
add_custom_command(
    TARGET generate_dependency_graph POST_BUILD
    COMMAND ${DOXYGEN_DOT_EXECUTABLE} ${_dot_file} -T png > "${_graph_dir}/${_image_file}"
    WORKING_DIRECTORY ${_graph_dir}
    COMMENT "Generating dependency graph image ${_graph_dir}/${_image_file} for "
            "${CMAKE_PROJECT_NAME} project"
)

add_custom_target(show_dependency_graph)
if(WIN32)
    set(_open_command start)
else()
    set(_open_command xdg-open)
endif()
add_custom_command(
    TARGET show_dependency_graph POST_BUILD
    COMMAND ${_open_command} "${_graph_dir}/${_image_file}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Show dependency graph image for ${CMAKE_PROJECT_NAME} project"
)

unset(_dot_file)
unset(_image_file)
unset(_graph_dir)
unset(_open_command)
