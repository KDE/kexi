add_subdirectory( dataviewcommon )
add_subdirectory( relations )
add_subdirectory( undo )
add_subdirectory( utils )

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    add_subdirectory( tableview )
endif()

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44023)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/widget/tableview
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/kexiutils/style
)

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

    properties/KexiPropertyPaneWidget.cpp
    properties/KexiCustomPropertyFactory.cpp
    properties/KexiCustomPropertyFactory_p.cpp
    properties/KexiObjectInfoWidget.cpp
    properties/KexiPropertyPaneLineEdit.cpp

    kexiquerydesignersqleditor.cpp
    kexiqueryparameters.cpp
    kexisectionheader.cpp
    kexidbdrivercombobox.cpp
    kexieditor.cpp
    KexiDataSourceComboBox.cpp
    kexicharencodingcombobox.cpp
    KexiDBCaptionPage.cpp
    KexiProjectSelectorWidget.cpp
    kexislider.cpp
    KexiServerDriverNotFoundMessage.cpp
    KexiNameWidget.cpp
    KexiNameDialog.cpp
    KexiFileWidgetInterface.cpp
    KexiFileRequester.cpp
    KexiStartupFileHandler.cpp
    KexiListView.cpp
    KexiWidgetWidthAnimator.cpp
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
    KexiDBCaptionPage.ui
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
    PUBLIC
        kexicore

        KPropertyWidgets

        KF5::TextEditor
        KF5::ConfigWidgets
)

if(KEXI_USE_KFILEWIDGET)
    target_link_libraries(kexiextendedwidgets
        PUBLIC KF5::KIOFileWidgets) # KFileWidget
else()
    target_link_libraries(kexiextendedwidgets
        PRIVATE KF5::KIOFileWidgets) # KFileWidget::getStartUrl, KFileFilterCombo
endif()

install(TARGETS kexiextendedwidgets  ${INSTALL_TARGETS_DEFAULT_ARGS})
