set(icons_qrc "${CMAKE_CURRENT_SOURCE_DIR}/kexi_breeze.qrc")
set(icons_rcc "${CMAKE_CURRENT_BINARY_DIR}/kexi_breeze.rcc")

add_custom_target(build_icons_qrc)

add_custom_command(TARGET build_icons_qrc
    COMMAND ${CMAKE_COMMAND} -E remove ${icons_qrc}
    COMMAND "${Qt5Core_RCC_EXECUTABLE}" --project -o "${icons_qrc}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Qt resource collection file ${icons_qrc}"
    VERBATIM)

add_custom_command(OUTPUT "${icons_rcc}"
    COMMAND "${Qt5Core_RCC_EXECUTABLE}" --binary --output "${icons_rcc}" "${icons_qrc}"
    DEPENDS "${icons_qrc}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building external Qt resource ${icons_rcc}"
    VERBATIM)

add_custom_target(build_icons_rcc DEPENDS "${icons_rcc}" SOURCES "${icons_qrc}")
add_dependencies(kexi build_icons_rcc)

install(FILES ${icons_rcc}
        DESTINATION "${ICONS_INSTALL_DIR}")
