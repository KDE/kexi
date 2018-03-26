include_directories(${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/widget)

# the main plugin
set(kexi_csvimportexportplugin_SRCS
   KexiCsvImportExportPlugin.cpp
   KexiCSVImportDialogModel.cpp
   kexicsvimportdialog.cpp
   kexicsvimportoptionsdlg.cpp
   kexicsvwidgets.cpp
   kexicsvexportwizard.cpp
   kexicsvexport.cpp
)

add_library(kexi_csvimportexportplugin MODULE ${kexi_csvimportexportplugin_SRCS})
kcoreaddons_desktop_to_json(kexi_csvimportexportplugin kexi_csvimportexportplugin.desktop)

target_link_libraries(kexi_csvimportexportplugin
    PRIVATE
        kexiextendedwidgets
        keximain

        KF5::Codecs
        KF5::KIOWidgets #for KIO::pixmapForUrl...
)

install(TARGETS kexi_csvimportexportplugin  DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
