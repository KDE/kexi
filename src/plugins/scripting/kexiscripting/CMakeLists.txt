remove_definitions( -DKDE_DEFAULT_DEBUG_AREA=44021 )
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44025)

simple_option(KEXI_SCRIPTING_DEBUG "Enable debugging for the scripting plugin" OFF)

if (KEXI_SCRIPTING_DEBUG)
    add_definitions(-DKEXI_SCRIPTING_DEBUG)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/widget
    ${CMAKE_SOURCE_DIR}/src/plugins/scripting/kexiscripting
)

# the main plugin
set(kexi_scriptplugin_SRCS
    kexiscriptpart.cpp
    kexiscripteditor.cpp
    kexiscriptdesignview.cpp
)

set(krossmodulekexidb_PART_SRCS
   ../kexidb/kexidbmodule.cpp
   ../kexidb/kexidbfield.cpp
   ../kexidb/kexidbfieldlist.cpp
   ../kexidb/kexidbschema.cpp
   ../kexidb/kexidbparser.cpp
   ../kexidb/kexidbcursor.cpp
   ../kexidb/kexidbconnectiondata.cpp
   ../kexidb/kexidbconnection.cpp
   ../kexidb/kexidbdriver.cpp )

   
qt5_wrap_cpp(kexi_scriptplugin_SRCS kexiscriptadaptor.h)

add_library(kexi_scriptplugin MODULE ${kexi_scriptplugin_SRCS} ${krossmodulekexidb_PART_SRCS})
kcoreaddons_desktop_to_json(kexi_scriptplugin kexi_scriptplugin.desktop)

target_link_libraries(kexi_scriptplugin
    PRIVATE
        kexiextendedwidgets
        kexicore
        Qt5::Qml
        KF5::KIOWidgets
        KDb 
        KF5::ConfigCore
        KF5::ConfigGui
)
install(TARGETS kexi_scriptplugin DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
