set(_target "kexi_breeze")
set(_theme "breeze")
set(_prefix)
set(_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resource)
set(_QRC_FILE "${_BASE_DIR}/${_target}.qrc")
set(_RCC_DIR "${CMAKE_BINARY_DIR}/bin/data/${KEXI_BASE_PATH}/icons")
set(_RCC_FILE "${_RCC_DIR}/${_target}.rcc")
include(kexi_breeze_files.cmake)

file(GLOB _PG_CONFIG_HEADERS "${PostgreSQL_INCLUDE_DIR}/pg_config*.h")

add_custom_target(copy_icons
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${_BASE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_BASE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_RCC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory . ${_BASE_DIR}
    # Don't include other files
    COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/colorize.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/kexi_breeze_files.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/update_kexi_breeze_list.sh
    COMMAND ${CMAKE_COMMAND} -E remove -f ${_BASE_DIR}/icons/${_prefix}/${_theme}/files.cmake
    DEPENDS "${_FILES}"
    SOURCES "${_FILES}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Copying icon files to ${_BASE_DIR}"
    VERBATIM
)

add_custom_command(OUTPUT "${status_rcc}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different calligra-logo-white-glow.png ${_BASE_DIR}
    DEPENDS "${status_qrc}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building external Qt resource ${status_rcc}"
    VERBATIM)

add_custom_target(colorize_icons
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/colorize.cmake
    DEPENDS "${_FILES}"
    SOURCES colorize.cmake
    WORKING_DIRECTORY "${_BASE_DIR}"
    COMMENT "Colorizing icons in ${_BASE_DIR}"
    VERBATIM
)
add_dependencies(colorize_icons copy_icons)

add_custom_target(build_qrc
    COMMAND ${Qt5Core_RCC_EXECUTABLE} --project -o "${CMAKE_CURRENT_BINARY_DIR}/${_target}.qrc"
    # avoid adding the .qrc file to rcc due to rcc misfeature
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_CURRENT_BINARY_DIR}/${_target}.qrc" "${_QRC_FILE}"
    BYPRODUCTS "${_QRC_FILE}"
    DEPENDS "${_FILES}"
    SOURCES "${_FILES}"
    WORKING_DIRECTORY "${_BASE_DIR}"
    COMMENT "Building Qt resource file ${_QRC_FILE}"
    VERBATIM
)
add_dependencies(build_qrc colorize_icons)

add_custom_target(build_rcc
    BYPRODUCTS "${_RCC_FILE}"
    COMMAND ${Qt5Core_RCC_EXECUTABLE} --compress 9 --threshold 0 --binary
            --output "${_RCC_FILE}" "${_QRC_FILE}"
    DEPENDS "${_QRC_FILE}" "${_FILES}"
    WORKING_DIRECTORY "${_BASE_DIR}"
    COMMENT "Building external Qt resource ${_RCC_FILE}"
    VERBATIM
)
add_dependencies(build_rcc build_qrc)

add_dependencies(kexi build_rcc)

install(FILES
        ${_RCC_FILE}
        DESTINATION "${ICONS_INSTALL_DIR}"
)

add_update_file_target(
    TARGET update_kexi_breeze
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/update_kexi_breeze_list.sh" kexi_breeze_files.cmake
    FILE kexi_breeze_files.cmake
    SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/update_kexi_breeze_list.sh"
)
add_dependencies(update_all_rcc update_${_target})

unset(_BASE_DIR)
unset(_QRC_FILE)
unset(_RCC_FILE)
unset(_target)
unset(_theme)
unset(_prefix)
