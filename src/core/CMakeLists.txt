add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44020)

set(kexicore_LIB_SRCS
   KexiGlobal.cpp
   kexi.cpp
   kexiaboutdata.cpp
   KexiMainWindowIface.cpp
   KexiStandardAction.cpp
   kexidbconnectionset.cpp
   kexiprojectset.cpp
   kexiactionproxy.cpp
   kexisharedactionhost.cpp
   kexiactioncategories.cpp
   kexiproject.cpp
   KexiWindow.cpp
   KexiWindowData.cpp
   KexiView.cpp
   kexipartmanager.cpp
   kexipartinfo.cpp
   kexipartitem.cpp
   kexipartbase.cpp
   kexipart.cpp
   kexipartguiclient.cpp
   kexiprojectdata.cpp
   KexiRecentProjects.cpp
   kexiinternalpart.cpp
   kexidragobjects.cpp
   kexistartupdata.cpp
   KexiCommandLineOptions.cpp
   kexiguimsghandler.cpp
   kexitextmsghandler.cpp
   kexidataiteminterface.cpp
   kexidbshortcutfile.cpp
   kexiblobbuffer.cpp
   #TODO KEXI3 kexistaticpart.cpp
   kexitabledesignerinterface.cpp
   kexisearchandreplaceiface.cpp
   kexitemplateloader.cpp
   KexiRecordNavigatorHandler.cpp
   KexiRecordNavigatorIface.cpp
   KexiSearchableModel.cpp
   KexiGroupButton.cpp #TODO belongs to widget/?
)

kexi_add_library(kexicore SHARED ${kexicore_LIB_SRCS})

generate_export_header(kexicore)

target_link_libraries(kexicore
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets

        KF5::CoreAddons
        KF5::XmlGui

        kexiutils

        KDb
        KPropertyWidgets
)

if(WIN32)
    target_include_directories(kexicore PUBLIC "${KDEWIN_INCLUDES}")
    target_link_libraries(kexicore PUBLIC ${KDEWIN_LIBRARIES})
endif()

<<<<<<< HEAD
=======
include_directories(${CMAKE_SOURCE_DIR}/src/kexiutils/style)

set_target_properties(kexicore PROPERTIES
    VERSION ${GENERIC_KEXI_LIB_VERSION} SOVERSION ${GENERIC_KEXI_LIB_SOVERSION}
)
>>>>>>> Move KexiStyle to kexiutils/style/, add KexiStyle::propertyPane(), add KexiPropertyPaneLineEditStyle
install(TARGETS kexicore  ${INSTALL_TARGETS_DEFAULT_ARGS})
#install(FILES kexihandler.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
