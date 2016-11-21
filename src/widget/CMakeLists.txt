add_subdirectory( dataviewcommon )
add_subdirectory( relations )
add_subdirectory( undo )
add_subdirectory( utils )

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    add_subdirectory( tableview )
endif()

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44023)

include_directories(${CMAKE_SOURCE_DIR}/src/widget/tableview ${CMAKE_SOURCE_DIR}/src/core)


########### next target ###############

set(kexiextendedwidgets_LIB_SRCS
    fields/KexiFieldComboBox.cpp
    fields/KexiFieldListModel.cpp
    fields/KexiFieldListModelItem.cpp
    fields/KexiFieldListView.cpp

    navigator/KexiProjectModel.cpp
    navigator/KexiProjectModelItem.cpp
    navigator/KexiProjectItemDelegate.cpp
    navigator/KexiProjectNavigator.cpp
    navigator/KexiProjectTreeView.cpp

    properties/KexiCustomPropertyFactory.cpp
    properties/KexiCustomPropertyFactory_p.cpp
    properties/KexiPropertyEditorView.cpp
    properties/KexiPropertyPaneViewBase.cpp

    kexiquerydesignersqleditor.cpp
    kexiqueryparameters.cpp
    kexisectionheader.cpp
    kexidbdrivercombobox.cpp
    kexieditor.cpp
    KexiDataSourceComboBox.cpp
    KexiObjectInfoLabel.cpp
    kexicharencodingcombobox.cpp
    KexiDBTitlePage.cpp
    KexiProjectSelectorWidget.cpp
    kexislider.cpp
    KexiServerDriverNotFoundMessage.cpp
    KexiNameWidget.cpp
    KexiNameDialog.cpp
    KexiFileRequester.cpp
    KexiStartupFileHandler.cpp
    KexiListView.cpp
  )

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
  list(APPEND kexiextendedwidgets_LIB_SRCS
    #navigator/KexiProjectListView.cpp
    #navigator/KexiProjectListViewItem.cpp
    kexidbconnectionwidget.cpp
# TODO replace use of KexiProjectListView and KexiProjectListViewList (with KexiProjectNavigator)
#      in kexiactionselectiondialog and remove them
    kexiprjtypeselector.cpp
    KexiConnectionSelectorWidget.cpp
    KexiFileDialog.cpp
    KexiPasswordWidget.cpp
    KexiDBPasswordDialog.cpp
  )
  if(KEXI_USE_KFILEWIDGET)
    list(APPEND kexiextendedwidgets_LIB_SRCS KexiFileWidget.cpp)
  endif()
  ki18n_wrap_ui(kexiextendedwidgets_LIB_SRCS
    KexiConnectionSelector.ui
    kexidbconnectionwidget.ui
    kexidbconnectionwidgetdetails.ui
    kexiprjtypeselector.ui
    KexiPasswordWidget.ui
  )
endif ()

ki18n_wrap_ui(kexiextendedwidgets_LIB_SRCS
    KexiDBTitlePage.ui
    KexiProjectSelector.ui
)

kexi_add_library(kexiextendedwidgets SHARED ${kexiextendedwidgets_LIB_SRCS})

generate_export_header(kexiextendedwidgets BASE_NAME kexiextwidgets)

target_link_libraries(kexiextendedwidgets
    PRIVATE
        kexidataviewcommon
        kexiguiutils

        KF5::TextWidgets # KTextEdit
        KF5::Codecs # KCharsets
        KF5::KIOFileWidgets # KFileWidget::getStartUrl()
    PUBLIC
        kexicore

        KPropertyWidgets

        KF5::TextEditor
)

if(KEXI_USE_KFILEWIDGET)
    target_link_libraries(kexiextendedwidgets
        PUBLIC KF5::KIOFileWidgets) # KFileWidget
endif()

install(TARGETS kexiextendedwidgets  ${INSTALL_TARGETS_DEFAULT_ARGS})
