# the main form widgets plugin

set(kexiforms_mainwidgetsplugin_SRCS
    KexiStandardFormWidgets.cpp
    KexiStandardContainerFormWidgets.cpp
    KexiMainFormWidgetsFactory.cpp
    kexiforms_mainwidgetsplugin.json
)

add_library(org.kexi-project.form.widgets.main MODULE ${kexiforms_mainwidgetsplugin_SRCS})

target_link_libraries(org.kexi-project.form.widgets.main
    PRIVATE
        kformdesigner
        kexiformutils
        kexicore
        kexiguiutils
        kexidataviewcommon
        kexidatatable
        kexiextendedwidgets

        KDb

        Qt5::Gui
)

install(TARGETS org.kexi-project.form.widgets.main DESTINATION ${KEXI_FORM_WIDGETS_PLUGIN_INSTALL_DIR})

add_subdirectory(pics)
