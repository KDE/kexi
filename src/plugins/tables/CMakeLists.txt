include_directories(${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/widget
 ${CMAKE_SOURCE_DIR}/src/widget/tableview ${CMAKE_SOURCE_DIR}/src/widget/properties)

# the main plugin
set(kexi_tableplugin_SRCS
    kexitablepart.cpp
    kexitabledesignerview.cpp
    kexitabledesigner_dataview.cpp
    kexitabledesignercommands.cpp
    kexitabledesignerview_p.cpp
    kexilookupcolumnpage.cpp
)

add_library(kexi_tableplugin MODULE ${kexi_tableplugin_SRCS})
kcoreaddons_desktop_to_json(kexi_tableplugin kexi_tableplugin.desktop)

target_link_libraries(kexi_tableplugin
    PRIVATE
        kexidataviewcommon
        kexidatatable
        kexiextendedwidgets
        kexiundo
)

install(TARGETS kexi_tableplugin DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
