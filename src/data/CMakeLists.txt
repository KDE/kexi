# Adds app icons, updates the source_var list
macro(kexi_add_app_icons source_var)
    set(_files
        ${CMAKE_CURRENT_SOURCE_DIR}/data/16-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/32-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/48-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/64-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/128-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/256-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/512-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/1024-apps-kexi.png
    )
    ecm_add_app_icon(source_var ICONS ${_files})
    list(APPEND ${source_var} ${_files})
    unset(_files)
endmacro()

# Adds metadata files to the source_var list
macro(kexi_add_app_metadata_files source_var)
    list(APPEND ${source_var}
         ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kexi.desktop.cmake
         ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kexi.appdata.xml)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kexi.desktop.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kexi-${KEXI_DISTRIBUTION_VERSION}.desktop)

    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kexi-${KEXI_DISTRIBUTION_VERSION}.desktop
            DESTINATION ${XDG_APPS_INSTALL_DIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/org.kde.kexi.appdata.xml
            DESTINATION ${CMAKE_INSTALL_METAINFODIR}
            RENAME org.kde.kexi-${KEXI_DISTRIBUTION_VERSION}.appdata.xml)
endmacro()
