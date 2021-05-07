# Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

add_custom_target(update_all_rcc
    COMMENT "Updating all file lists for rcc icons files"
)

# Builds and install an rcc file with icons.
# - resource subdirectory in the current build subdir is created
# - ${_target}.qrc is created based on icons/${_theme}/files.cmake
# - ${_target}.rcc is generated using rcc-qt5
# - if _prefix is not empty, icons are placed in icons/${_prefix}/${_theme} path (useful for plugins)
# - dependency for the parent target ${_parent_target} is added
# - the .rcc file is installed to ${ICONS_INSTALL_DIR}
# - update_${_target} target is added for requesting update of icons/${_theme}/files.cmake
# - adds a update_all_rcc target that executes commands for all targets created with kexi_add_icons_rcc_file()
macro(kexi_add_icons_rcc_file _target _parent_target _theme _prefix)
    set(_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resource)
    set(_QRC_FILE "${_BASE_DIR}/${_target}.qrc")
    set(_RCC_DIR "${CMAKE_BINARY_DIR}/bin/data/${KEXI_BASE_PATH}/icons")
    set(_RCC_FILE "${_RCC_DIR}/${_target}.rcc")
    include(icons/${_theme}/files.cmake)

    add_custom_target(${_target}_copy_icons
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${_BASE_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_BASE_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_RCC_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory icons/${_theme} ${_BASE_DIR}/icons/${_prefix}/${_theme}
        COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/CMakeLists.txt
        COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/icons/${_prefix}/${_theme}/files.cmake
        DEPENDS "${_FILES}"
        SOURCES "${_FILES}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Copying icon files to ${_BASE_DIR}"
        VERBATIM
    )

    add_custom_target(${_target}_build_qrc
        BYPRODUCTS "${_QRC_FILE}"
        COMMAND ${Qt5Core_RCC_EXECUTABLE} --project -o "${CMAKE_CURRENT_BINARY_DIR}/${_target}.qrc"
        # avoid adding the .qrc file to rcc due to rcc misfeature
        COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_CURRENT_BINARY_DIR}/${_target}.qrc" "${_QRC_FILE}"
        DEPENDS "${_FILES}"
        SOURCES "${_FILES}"
        WORKING_DIRECTORY "${_BASE_DIR}"
        COMMENT "Building Qt resource file ${_QRC_FILE}"
        VERBATIM
    )
    add_dependencies(${_target}_build_qrc ${_target}_copy_icons)

    add_custom_target(${_target}_build_rcc
        BYPRODUCTS "${_RCC_FILE}"
        COMMAND ${Qt5Core_RCC_EXECUTABLE} --compress 9 --threshold 0 --binary
                --output "${_RCC_FILE}" "${_QRC_FILE}"
        DEPENDS "${_QRC_FILE}" "${_FILES}"
        WORKING_DIRECTORY "${_BASE_DIR}"
        COMMENT "Building external Qt resource ${_RCC_FILE}"
        VERBATIM
    )
    add_dependencies(${_target}_build_rcc ${_target}_build_qrc)

    add_dependencies(${_parent_target} ${_target}_build_rcc)

    install(FILES
            ${_RCC_FILE}
            DESTINATION "${ICONS_INSTALL_DIR}"
    )

    add_update_file_target(
        TARGET update_${_target}
        COMMAND "${PROJECT_SOURCE_DIR}/cmake/modules/update_icon_list.sh"
                ${_theme} icons/${_theme}/files.cmake
        FILE ${_target}_files.cmake
        SOURCES "${PROJECT_SOURCE_DIR}/cmake/modules/update_icon_list.sh"
    )
    add_dependencies(update_all_rcc update_${_target})

    unset(_BASE_DIR)
    unset(_QRC_FILE)
    unset(_RCC_FILE)
endmacro()
