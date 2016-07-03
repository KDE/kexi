include_directories(${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_BINARY_DIR}/src/widget
)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44000)

add_subdirectory(txt)

find_package(MySQL)
macro_log_feature(MYSQL_FOUND "libmysqlclient" "MySQL Client Library" "http://www.mysql.com" FALSE "" "Required by Kexi MySQL migration driver")
if (MYSQL_FOUND)
  add_subdirectory( mysql )
endif ()

find_package(KexiPostgreSQL)
macro_log_feature(POSTGRESQL_FOUND "libpq" "C application programmer's interface to PostgreSQL" "http://www.postgresql.org" FALSE "" "Required by Kexi PostgreSQL migration driver")
if (POSTGRESQL_FOUND)
  add_subdirectory( pqxx )
endif ()

find_package(FreeTDS)
macro_log_feature(FREETDS_FOUND "FreeTDS" "Open source implementation of the TDS (Tabular Data Stream) protocol" "http://www.freetds.org" FALSE "" "Required by Kexi Sybase migration driver")
if(FREETDS_FOUND)
  add_subdirectory(sybase)
endif()

find_package(XBase)
macro_log_feature(XBASE_FOUND "XBase" "XBase compatible C++ class library" "http://linux.techass.com/projects/xdb" FALSE "" "Required by Kexi XBase migration driver")
if(XBASE_FOUND)
  add_subdirectory(xbase)
endif()

find_package(GLIB2)
macro_log_feature(GLIB2_FOUND "GLIB2" "Common C routines used by GTK+ and other libs" "http://www.gtk.org" FALSE "" "Required by Kexi MS Access migration driver")
if (GLIB2_FOUND)
    add_subdirectory( mdb )
endif()

add_subdirectory(tests)

########### next target ###############

set(keximigrate_LIB_SRCS AlterSchemaTableModel.cpp
   keximigrate.cpp
   keximigratedata.cpp
   migratemanager.cpp
   importwizard.cpp
   importtablewizard.cpp
   importoptionsdlg.cpp
   AlterSchemaWidget.cpp)

add_library(keximigrate SHARED ${keximigrate_LIB_SRCS})

target_link_libraries(
    keximigrate
    kexicore
    kexiextendedwidgets
    kexiutils

    KDb

    Qt5::Gui
    )

generate_export_header(keximigrate)

set_target_properties(keximigrate PROPERTIES VERSION ${GENERIC_KEXI_LIB_VERSION} SOVERSION ${GENERIC_KEXI_LIB_SOVERSION} )
install(TARGETS keximigrate  ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

if(FALSE) # TODO: install when we move to independent place
    install( FILES  keximigrate.h keximigratedata.h migratemanager.h  DESTINATION ${INCLUDE_INSTALL_DIR}/kexidb COMPONENT Devel)
endif()
