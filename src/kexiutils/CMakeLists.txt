add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44024)

include_directories(completer)

set(kexiutils_LIB_SRCS
  utils.cpp
  FontSettings_p.cpp
  InternalPropertyMap.cpp
  SmallToolButton.cpp
  KexiCommandLinkButton.cpp
  FlowLayout.cpp
  transliteration_table.cpp
  kmessagewidget.cpp
  KexiContextMessage.cpp
  KexiTitleLabel.cpp
  KexiLinkWidget.cpp
  KexiLinkButton.cpp
  KexiCloseButton.cpp
  KexiAssistantPage.cpp
  KexiAssistantWidget.cpp
  KexiAnimatedLayout.cpp
  KexiCategorizedView.cpp
  KexiTester.cpp
  KexiJsonTrader.cpp
  KexiPushButton.cpp
  KexiFadeWidgetEffect.cpp
  KexiPluginMetaData.cpp

  completer/KexiCompleter.cpp

  style/KexiStyle.cpp
  style/KexiPropertyPaneLineEditStyle.cpp
)

if(SHOULD_BUILD_KEXI_MOBILE_APP)
  if (KEXI_DEBUG_GUI)
    list(APPEND kexiutils_LIB_SRCS
      debuggui.cpp
    )
  endif ()
endif ()

if(BUILD_TESTING)
    list(APPEND kexiutils_LIB_SRCS
      KexiTestHandler.cpp
    )
endif()

kexi_add_library(kexiutils SHARED ${kexiutils_LIB_SRCS})

set(kexiutils_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(kexiutils
    PUBLIC "$<BUILD_INTERFACE:${kexiutils_INCLUDE_DIRS}>"
)

target_link_libraries(kexiutils
    PUBLIC
        KF5::IconThemes
        KF5::WidgetsAddons
        KF5::ConfigWidgets # KStandardAction KColorScheme
        KF5::GuiAddons # KColorUtils
        KF5::I18n
        KF5::ItemViews # KCategorizedView KCategoryDrawer
        KDb
        KPropertyWidgets

    PRIVATE
        Qt5::Svg
        KF5::KIOFileWidgets # KFileWidget::getStartUrl(), KRecentDirs
)
if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    target_link_libraries(kexiutils
        PUBLIC
            KF5::KIOWidgets #for KRun...
    )
#target_link_libraries(kexiutils LINK_INTERFACE_LIBRARIES KF5::KIOWidgets)
endif()

if(BUILD_TESTING)
    target_link_libraries(kexiutils
        PRIVATE
            Qt5::Test
    )
endif()

generate_export_header(kexiutils)

install(TARGETS kexiutils  ${INSTALL_TARGETS_DEFAULT_ARGS})

if(FALSE) # TODO: install when we move to independent place
    install( FILES  tristate.h utils.h kexiutils_export.h kexiutils_global.h
    InternalPropertyMap.h
    SmallToolButton.h FlowLayout.h
    kmessagewidget.h KexiContextMessage.h KexiTitleLabel.h KexiAssistantPage.h
    KexiAssistantWidget.h KexiAnimatedLayout.h
    DESTINATION ${INCLUDE_INSTALL_DIR}/kexiutils COMPONENT Devel)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
