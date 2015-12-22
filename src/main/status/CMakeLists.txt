set(status_qrc "${CMAKE_CURRENT_SOURCE_DIR}/status.qrc")
set(status_rcc "${CMAKE_CURRENT_BINARY_DIR}/status.rcc")

qt5_add_binary_resources(build_status_rcc "${status_qrc}" OPTIONS -binary DESTINATION "${status_rcc}")

add_custom_target(build_status_rcc DEPENDS "${status_rcc}" SOURCES "${status_qrc}")

add_dependencies(keximain build_status_rcc)

install(FILES
        status.ui
        contribution_details.ui
        contribution_help.ui
        status_strings.ui
        ${status_rcc}
        DESTINATION "${DATA_INSTALL_DIR}/kexi/status/${KEXI_STABLE_VERSION_MAJOR}.${KEXI_STABLE_VERSION_MINOR}.0")
