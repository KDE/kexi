add_definitions(-DTRANSLATION_DOMAIN=\"keximigrate_spreadsheet\")

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs
    ${KOMAIN_INCLUDES}
)


########### next target ###############

set(keximigrate_spreadsheet_PART_SRCS
    spreadsheetmigrate.cpp
    Messages.sh
)


add_library(keximigrate_spreadsheet MODULE ${keximigrate_spreadsheet_PART_SRCS})

kcoreaddons_desktop_to_json(keximigrate_spreadsheet keximigrate_spreadsheet.desktop)

target_link_libraries(keximigrate_spreadsheet
    keximigrate
    kexiutils
    calligrasheetscommon

    Predicate
)

install(TARGETS keximigrate_spreadsheet DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
