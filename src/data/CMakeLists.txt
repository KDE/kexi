# Adds app icons, updates target sources for kexi target
macro(kexi_add_app_icons)
    set(_sizes 16 32 48 64 128 256)
    if(NOT APPLE)
        # Add larger icons. Not on Mac because iconutil is limited to 256x256 on at least
        # OS X 10.9; that size is probably large enough not to bother testing for OS version here.
        list(APPEND _sizes 512 1024)
    endif()
    unset(_copy_commands)
    unset(_source_files)
    unset(_files)
    foreach(_size ${_sizes})
        list(APPEND _copy_commands
             COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/data/${_size}-apps-kexi.png
             ${CMAKE_CURRENT_BINARY_DIR}/data/${_size}-apps-kexi-${KEXI_DISTRIBUTION_VERSION}.png)
        list(APPEND _source_files ${CMAKE_CURRENT_SOURCE_DIR}/data/${_size}-apps-kexi.png)
        list(APPEND _files ${CMAKE_CURRENT_BINARY_DIR}/data/${_size}-apps-kexi-${KEXI_DISTRIBUTION_VERSION}.png)
    endforeach()
    add_custom_target(copy_app_icons ${_copy_commands}
        DEPENDS "${_source_files}"
        SOURCES "${_source_files}"
        COMMENT "Preparing app icon files for version ${KEXI_DISTRIBUTION_VERSION}"
        VERBATIM
    )
    add_dependencies(kexi copy_app_icons)
    ecm_add_app_icon(
        _source_var ICONS ${_source_files}) # Win/Mac: still install copied KEXI files (_files)
                                            # BUT add the original ones (_source_files) to the binary
                                            # because the copied ones do not exist yet before
                                            # ecm_add_app_icon() gets called
    ecm_install_icons(ICONS ${_files}
                      DESTINATION ${ICON_INSTALL_DIR}
                      THEME hicolor)
    target_sources(kexi PRIVATE ${_source_var})
    unset(_files)
    unset(_source_files)
    unset(_sizes)
    unset(_source_var)
endmacro()

# Adds metadata files to the source_var list
macro(kexi_add_app_metadata_files source_var)

    # Check if the <binary> tag has proper binary version.
    set(_appdata ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kexi.appdata.xml)
    file(READ ${_appdata} _content)
    set(_binary_spec "<binary>kexi-${KEXI_DISTRIBUTION_VERSION}</binary>")
    string(FIND "${_content}" "${_binary_spec}" _pos)
    if(${_pos} EQUAL -1)
        message(FATAL_ERROR "Missing or invalid specification of KEXI binary \"${_binary_spec}\" in \"${_appdata}\". Please fix it.")
    endif()
    unset(_binary_spec)
    unset(_pos)

    # Keep org.kde.kexi.appdata.xml in-source to satisfy the kde.org/applications scanner.
    # Then install with proper coinstallable name org.kde.kexi-${KEXI_DISTRIBUTION_VERSION}.appdata.xml.
    install(FILES ${_appdata}
            RENAME org.kde.kexi-${KEXI_DISTRIBUTION_VERSION}.appdata.xml
            DESTINATION ${CMAKE_INSTALL_METAINFODIR})
    install(PROGRAMS org.kde.kexi.desktop
            RENAME org.kde.kexi-${KEXI_DISTRIBUTION_VERSION}.desktop
            DESTINATION ${XDG_APPS_INSTALL_DIR})
endmacro()
