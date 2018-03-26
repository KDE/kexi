include_directories( ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_SOURCE_DIR}/src/migration)

set(kexi_migrationplugin_PART_SRCS keximigrationpart.cpp)

add_library(kexi_migrationplugin MODULE ${kexi_migrationplugin_PART_SRCS})
kcoreaddons_desktop_to_json(kexi_migrationplugin kexi_migrationplugin.desktop)

target_link_libraries(kexi_migrationplugin
    PRIVATE
        keximigrate
)

install(TARGETS kexi_migrationplugin DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
