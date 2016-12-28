# Adds app icons, updates the source_var list
macro(kexi_add_app_icons source_var)
    set(_files
        ${CMAKE_CURRENT_SOURCE_DIR}/data/16-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/32-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/48-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/64-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/128-apps-kexi.png
        ${CMAKE_CURRENT_SOURCE_DIR}/data/256-apps-kexi.png
    )
    if(NOT APPLE)
        # Add larger icons. Not on Mac because iconutil is limited to 256x256 on at least
        # OS X 10.9; that size is probably large enough not to bother testing for OS version here.
        list(APPEND _files
            ${CMAKE_CURRENT_SOURCE_DIR}/data/512-apps-kexi.png
            ${CMAKE_CURRENT_SOURCE_DIR}/data/1024-apps-kexi.png
        )
    endif()
    ecm_add_app_icon(${source_var} ICONS ${_files})
    list(APPEND ${source_var} ${_files})
    unset(_files)
endmacro()

install(PROGRAMS org.kde.kexi.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.kexi.appdata.xml DESTINATION ${CMAKE_INSTALL_METAINFODIR})
