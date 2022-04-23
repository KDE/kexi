set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

check_function_exists("uname" HAVE_UNAME)

simple_option(KEXI_SHOW_UNFINISHED "Show unfinished features in KEXI. Thus is useful for testing \
but may confuse end-user." OFF)
simple_option(KEXI_SHOW_UNIMPLEMENTED "Forces to show menu entries and dialogs just to give \
impression about development plans for KEXI. Only recommended for test/development versions." OFF)

# Extra GUI features
if(NOT KEXI_MOBILE)
    simple_option(KEXI_AUTORISE_TABBED_TOOLBAR "Experimental: Autorise the main tabbed toolbar in KEXI" OFF)
    if(WIN32 OR APPLE)
        set(_KEXI_USE_KFILEWIDGET_DEFAULT OFF)
    else()
        set(_KEXI_USE_KFILEWIDGET_DEFAULT ON)
    endif()
    simple_option(KEXI_USE_KFILEWIDGET "Use KFileWidget-based inline file browser in KEXI. If OFF, a simple \
replacement file widget with native file dialogs is used. ON by default on UNIX, OFF by default on Windows \
and macOS. \
Note: Non-plasma Linux desktops still default to the simple replacement at runtime."
           ${_KEXI_USE_KFILEWIDGET_DEFAULT}
    )
endif(NOT KEXI_MOBILE)

# Experimental:
simple_option(KEXI_SCRIPTS_SUPPORT "Experimental: Enable scripting in KEXI" OFF)
if(KEXI_SCRIPTS_SUPPORT)
    set(REQUIRED_QTQML_VERSION 5.8.0) # >= 5.8 because of QJSEngine::newQMetaObject()
    find_package(Qt5Qml ${REQUIRED_QTQML_VERSION} REQUIRED)
    set_package_properties(Qt5Qml PROPERTIES
                        DESCRIPTION "A framework for developing applications and libraries with the QML and JavaScript language."
                        URL "http://qt.io"
                        TYPE REQUIRED PURPOSE "Required by KEXI scripting (JavaScript)"
    )
endif()

# Broken:
simple_option(KEXI_FORM_CURSOR_PROPERTY_SUPPORT "Broken: Enable \"cursor\" property in the form designer" OFF)
simple_option(KEXI_SHOW_CONTEXT_HELP "Broken: Enable context help in KEXI main window" OFF)
simple_option(KEXI_QUICK_PRINTING_SUPPORT "Broken: Enable print/print preview/print setup for tables/queries in the project navigator" OFF)
simple_option(KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT "Broken: Enable \"auto field\" form widget in the form designer" OFF)
# OFF because we need to replace it with QTreeWidget which uses very different API compared to Q3ListView. Re-add QTreeWidget?
simple_option(KEXI_LIST_FORM_WIDGET_SUPPORT "Broken: Enable \"list\" form widget in the form designer" OFF)
simple_option(KEXI_PIXMAP_COLLECTIONS_SUPPORT "Broken: Enable support for pixmap collections" OFF)

# Not available:
simple_option(KEXI_MACROS_SUPPORT "Experimental: Enable macros in KEXI" OFF)
if(KEXI_MACROS_SUPPORT) # temp.
    message(FATAL_ERROR "Macros are not yet available.")
endif()
simple_option(KEXI_TABLE_PRINT_SUPPORT "Experimental: Enable printing of tabular view in KEXI" OFF) # broken since KEXI 2
if(KEXI_TABLE_PRINT_SUPPORT) # temp.
    message(FATAL_ERROR "Table printing is not yet available.")
endif()
simple_option(KEXI_PROJECT_TEMPLATES "Experimental: Enable support for project templates in KEXI" OFF) # broken since KEXI 2
if(KEXI_PROJECT_TEMPLATES) # temp.
    message(FATAL_ERROR "Project templates are not yet available.")
endif()

#See commit 1e433a54cd9, left here for reference
#option(KEXI_SQLITE_MIGRATION "If defined, SQLite3 migration to some newer format is possible. Users can see a suitable question on app's startup." OFF)

add_definitions(-DTRANSLATION_DOMAIN=\"kexi\")

#no default: add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44010)

macro_bool_to_01(SHOULD_BUILD_KEXI_DESKTOP_APP KEXI_DESKTOP)
macro_bool_to_01(SHOULD_BUILD_KEXI_MOBILE_APP KEXI_MOBILE)
configure_file(config-kexi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kexi.h )
configure_file(KexiVersion.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/KexiVersion.h)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/core
)

add_subdirectory( kexiutils )
add_subdirectory( core )

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    add_subdirectory( widget )
    add_subdirectory( main )
    add_subdirectory( formeditor )
    add_subdirectory( migration )
endif()

add_subdirectory( data )
add_subdirectory( plugins )

if (BUILD_TESTING)
#TODO KEXI3 add_subdirectory( tests )
endif()

########### next target ###############

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    set(kexi_SRCS
        main.cpp
        Messages.sh

        # non-source:
        ${CMAKE_SOURCE_DIR}/kundo2_aware_xgettext.sh
        Mainpage.dox
        Messages.sh
    )
    kexi_add_app_metadata_files(kexi_SRCS)
    kexi_add_executable(kexi ${kexi_SRCS})
    kexi_add_app_icons()
    target_link_libraries(kexi
        PRIVATE
            keximain
    )
    install(TARGETS kexi ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

    add_subdirectory( pics )
endif()

if(SHOULD_BUILD_KEXI_MOBILE_APP)
#  add_subdirectory( mobile )
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
