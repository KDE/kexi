set(status_qrc "${CMAKE_CURRENT_SOURCE_DIR}/status.qrc")
set(status_prefix "${KEXI_BASE_PATH}/status")
set(status_rcc_dir "${CMAKE_BINARY_DIR}/bin/data/${status_prefix}")
set(status_rcc "${status_rcc_dir}/status.rcc")

add_custom_target(makedir_status_rcc
    COMMAND ${CMAKE_COMMAND} -E make_directory ${status_rcc_dir}
)

if(${Qt5Core_VERSION} VERSION_LESS 5.5.0)
    add_custom_command(OUTPUT "${status_rcc}"
        COMMAND "${Qt5Core_RCC_EXECUTABLE}" --binary --verbose --output "${status_rcc}" "${status_qrc}"
        DEPENDS "${status_qrc}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building external Qt resource ${status_rcc}"
        VERBATIM)
    add_custom_target(build_status_rcc DEPENDS "${status_rcc}" SOURCES "${status_qrc}")
else()
    qt5_add_binary_resources(build_status_rcc "${status_qrc}" OPTIONS -binary DESTINATION "${status_rcc}")
endif()

add_dependencies(build_status_rcc makedir_status_rcc)
add_dependencies(keximain build_status_rcc)

install(FILES
        status.ui
        contribution_details.ui
        contribution_help.ui
        status_strings.ui
        ${status_rcc}
        DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/${status_prefix}")
