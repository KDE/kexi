include_directories(
    ${CMAKE_SOURCE_DIR}/src/formeditor
    ${CMAKE_SOURCE_DIR}/src/core
)

set(kexiforms_mapwidgetplugin_SRCS
    MapBrowserWidget.cpp
    MapBrowserFactory.cpp
    Messages.sh
)

add_library(kexiforms_mapwidgetplugin MODULE ${kexiforms_mapwidgetplugin_SRCS})
kcoreaddons_desktop_to_json(kexiforms_mapwidgetplugin kexiforms_mapwidgetplugin.desktop)

target_link_libraries(kexiforms_mapwidgetplugin
    kformdesigner
    kexiguiutils
    kexidatatable
    kexiextendedwidgets
    kexiformutils
    kexidataviewcommon

    Marble
    Qt5::Xml
)

install(TARGETS kexiforms_mapwidgetplugin DESTINATION ${KEXI_FORM_WIDGETS_PLUGIN_INSTALL_DIR})

ecm_install_icons(ICONS
    16-actions-map_browser.png
    22-actions-map_browser.png
    DESTINATION ${KDE_INSTALL_DATADIR}/kexi/icons
    THEME hicolor
)
