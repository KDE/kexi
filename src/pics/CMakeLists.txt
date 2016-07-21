set(_RESOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/resource)
set(_QRC_FILE "kexi_breeze.qrc")
set(_RCC_FILE "kexi_breeze.rcc")
include(kexi_breeze_files.cmake)

add_custom_target(copy_icons
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${_RESOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${_RESOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory . ${_RESOURCE_DIR}
    DEPENDS "${_FILES}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Copying icon files to ${_RESOURCE_DIR}"
    VERBATIM
)

add_custom_target(colorize_icons
    COMMAND ${CMAKE_COMMAND} -P colorize.cmake
    DEPENDS "${_FILES}"
    SOURCES colorize.cmake
    WORKING_DIRECTORY "${_RESOURCE_DIR}"
    COMMENT "Colorizing icons in ${_RESOURCE_DIR}"
    VERBATIM
)
add_dependencies(colorize_icons copy_icons)

add_custom_target(build_qrc
    COMMAND ${Qt5Core_RCC_EXECUTABLE} --project -o "${_QRC_FILE}"
    DEPENDS "${_FILES}"
    WORKING_DIRECTORY "${_RESOURCE_DIR}"
    COMMENT "Building Qt resource file ${_RESOURCE_DIR}/${_QRC_FILE}"
    VERBATIM
)
add_dependencies(build_qrc colorize_icons)

add_custom_target(build_rcc
    COMMAND ${Qt5Core_RCC_EXECUTABLE} --compress 9 --threshold 0 --binary
            --output "${_RCC_FILE}" "${_QRC_FILE}"
    DEPENDS "${_RESOURCE_DIR}/${_QRC_FILE}" "${_FILES}"
    WORKING_DIRECTORY "${_RESOURCE_DIR}"
    COMMENT "Building external Qt resource ${_RESOURCE_DIR}/${_RCC_FILE}"
    VERBATIM
)
add_dependencies(build_rcc build_qrc)

add_dependencies(kexi build_rcc)

install(FILES
        ${_RESOURCE_DIR}/${_RCC_FILE}
        DESTINATION "${ICONS_INSTALL_DIR}"
)
