include_directories( ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/widget/utils ${CMAKE_SOURCE_DIR}/src/widget
    ${CMAKE_SOURCE_DIR}/src/formeditor)

# the form plugin
set(kexi_formplugin_SRCS kexiforms.cpp)

add_library(kexi_formplugin MODULE ${kexi_formplugin_SRCS})
kcoreaddons_desktop_to_json(kexi_formplugin kexi_formplugin.desktop)

target_link_libraries(kexi_formplugin
    kexiguiutils
    kexidatatable
    kexiextendedwidgets
    kformdesigner
    kexiformutils

    KPropertyWidgets
)

install(TARGETS kexi_formplugin DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})

# the form utils lib
set(kexiformutils_LIB_SRCS
 #  kexiformdataiteminterface.cpp
   kexidataawarewidgetinfo.cpp
   KexiFormScrollAreaWidget.cpp
   kexiformscrollview.cpp
   kexidbtextwidgetinterface.cpp
   kexiformmanager.cpp
   kexidatasourcepage.cpp
   kexiformpart.cpp
   kexiformview.cpp
   kexidbfactorybase.cpp

  widgets/kexidbutils.cpp
  widgets/kexidbautofield.cpp
  widgets/kexidbform.cpp
  widgets/kexidblabel.cpp
  widgets/kexidbimagebox.cpp
  widgets/KexiDBPushButton.cpp
  widgets/kexiframe.cpp
  widgets/kexidblineedit.cpp
  widgets/kexidbcheckbox.cpp
  widgets/kexidbtextedit.cpp
  widgets/kexidbcombobox.cpp
  widgets/kexidbcommandlinkbutton.cpp
  widgets/kexidbslider.cpp
  widgets/kexidbprogressbar.cpp
  widgets/kexidbdatepicker.cpp
)

    #obsolete    widgets/kexidbdoublespinbox.cpp
    #obsolete    widgets/kexidbintspinbox.cpp

kexi_add_library(kexiformutils SHARED ${kexiformutils_LIB_SRCS})

generate_export_header(kexiformutils)

target_link_libraries(kexiformutils
    PRIVATE
        kexiextendedwidgets
        kformdesigner
        kexidataviewcommon
        kexidatatable
        kexiguiutils

        KPropertyWidgets

        Qt5::Gui
        Qt5::Xml
)
set(kexiformutils_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/plugins/forms
    ${CMAKE_SOURCE_DIR}/src/plugins/forms/widgets
)
target_include_directories(kexiformutils
    PUBLIC "$<BUILD_INTERFACE:${kexiformutils_INCLUDE_DIRS}>"
)

install(TARGETS kexiformutils  ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(widgets)
