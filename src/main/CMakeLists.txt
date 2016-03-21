#TODO add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44019)

include_directories(${CMAKE_SOURCE_DIR}/src/core
${CMAKE_SOURCE_DIR}/src/kexiutils
${CMAKE_SOURCE_DIR}/src/main ${CMAKE_SOURCE_DIR}/src/main/startup
${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_BINARY_DIR}/src/widget)

set(QT_USE_QTUITOOLS true)

set(keximain_LIB_SRCS
    KexiMainWindow.cpp
    KexiMainWindow_p.cpp
    KexiMenuWidget.cpp
    kexifinddialog.cpp
    KexiSearchLineEdit.cpp
    KexiUserFeedbackAgent.cpp
    KexiBugReportDialog.cpp
    KexiModeSelector.cpp

    startup/KexiNewProjectAssistant.cpp
    startup/KexiOpenProjectAssistant.cpp
    startup/KexiWelcomeAssistant.cpp
    startup/KexiWelcomeStatusBar.cpp
    startup/KexiImportExportAssistant.cpp
    startup/KexiStartupDialog.cpp
    startup/KexiStartup.cpp
    startup/KexiTemplatesModel.cpp
    startup/KexiRecentProjectsModel.cpp
    startup/KexiAssistantMessageHandler.cpp
    startup/KexiPasswordPage.cpp

    #todo printing/kexisimpleprintingengine.cpp
    #todo printing/kexisimpleprintingpagesetup.cpp
    #todo printing/kexisimpleprintingpart.cpp
    #todo printing/kexisimpleprintpreviewwindow.cpp
)

ki18n_wrap_ui(keximain_LIB_SRCS
    kexifinddialog.ui
    startup/KexiProjectStorageTypeSelectionPage.ui
    startup/KexiServerDBNamePage.ui
    startup/KexiMainImportExportPage.ui
)

kexi_add_library(keximain SHARED ${keximain_LIB_SRCS})

generate_export_header(keximain)

target_link_libraries(keximain
PUBLIC
    kexicore

PRIVATE
    kexiextendedwidgets
    kexiguiutils

    KF5::GuiAddons

    Qt5::UiTools
)
if(HAVE_KCRASH)
  target_link_libraries(keximain
      PRIVATE
          KF5::Crash
  )
endif()

target_compile_definitions(keximain PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
install(TARGETS keximain  ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(status)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
