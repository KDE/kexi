include_directories(${CMAKE_SOURCE_DIR}/src/core
  ${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_BINARY_DIR}/src/widget
)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=44000)

simple_option(KEXI_MIGRATEMANAGER_DEBUG "Enable debugging for the migrate driver manager" OFF)

set(KEXI_MIGRATE_PLUGIN_INSTALL_DIR ${KEXI_PLUGIN_INSTALL_DIR}/migrate)

# -----------------------
function(build_and_install_kexi_migrate_driver _name _srcs _extra_libs _includes _defines)
    set(_target keximigrate_${_name})
    ecm_create_qm_loader(_srcs ${_target}_qt)
    add_library(${_target} MODULE ${_srcs})
    target_link_libraries(${_target}
        PUBLIC
            keximigrate
            ${_extra_libs}
    )
    target_include_directories(${_target} PRIVATE ${_includes})
    target_compile_definitions(${_target} PRIVATE ${_defines})
    # Needed for examples and autotests:
    set_target_properties(${_target}
                          PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/plugins/migrate")

    install(TARGETS ${_target} DESTINATION ${KEXI_MIGRATE_PLUGIN_INSTALL_DIR})
endfunction()
# -----------------------

add_subdirectory(tsv)

find_package(MySQL)
set_package_properties(MySQL PROPERTIES TYPE RECOMMENDED
                       PURPOSE "Required by Kexi MySQL migration driver")
if(MySQL_FOUND)
    add_subdirectory(mysql)
endif()

find_package(PostgreSQL)
set_package_properties(PostgreSQL PROPERTIES TYPE RECOMMENDED
                       PURPOSE "Required by Kexi PostgreSQL migration driver")
if(PostgreSQL_FOUND)
    add_subdirectory(postgresql)
endif()

if(false) # TODO KEXI3
find_package(FreeTDS)
set_package_properties(FreeTDS PROPERTIES
    DESCRIPTION "Open source implementation of the TDS (Tabular Data Stream) protocol"
    URL "http://www.freetds.org"
    TYPE RECOMMENDED
    PURPOSE "Required by Kexi Sybase migration driver"
)
if(FREETDS_FOUND)
  add_subdirectory(sybase)
endif()

find_package(XBase)
set_package_properties(XBase PROPERTIES
    DESCRIPTION "XBase compatible C++ class library"
    URL "http://linux.techass.com/projects/xdb"
    TYPE RECOMMENDED
    PURPOSE "Required by Kexi XBase migration driver"
)
if(XBASE_FOUND)
  add_subdirectory(xbase)
endif()

endif() # KEXI3

find_package(GLIB2)
set(_REQUIRED_BY_MDB "Required by Kexi MS Access migration driver")
set_package_properties(GLIB2 PROPERTIES TYPE RECOMMENDED PURPOSE "${_REQUIRED_BY_MDB}")

find_package(Iconv)
set_package_properties(Iconv PROPERTIES TYPE RECOMMENDED PURPOSE "${_REQUIRED_BY_MDB}")

if(GLIB2_FOUND AND Iconv_FOUND)
    add_subdirectory(mdb)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

########### next target ###############

set(keximigrate_LIB_SRCS AlterSchemaTableModel.cpp
   KexiMigratePluginMetaData.cpp
   keximigrate.cpp
   keximigratedata.cpp
   KexiSqlMigrate.cpp
   migratemanager.cpp
   importwizard.cpp
   importtablewizard.cpp
   importoptionsdlg.cpp
   AlterSchemaWidget.cpp)

kexi_add_library(keximigrate SHARED ${keximigrate_LIB_SRCS})

target_link_libraries(keximigrate
    PUBLIC
        kexiextendedwidgets
)

generate_export_header(keximigrate)

install(TARGETS keximigrate  ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

if(FALSE) # TODO: install when we move to independent place
    install(FILES KexiMigratePluginMetaData.h keximigrate.h keximigratedata.h KexiSqlMigrate.h migratemanager.h
            DESTINATION ${INCLUDE_INSTALL_DIR}/kexidb COMPONENT Devel
    )
endif()
