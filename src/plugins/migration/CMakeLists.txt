
include_directories( ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_SOURCE_DIR}/src/migration)


########### next target ###############

set(kexihandler_migration_PART_SRCS keximigrationpart.cpp )


add_library(kexihandler_migration MODULE ${kexihandler_migration_PART_SRCS})



target_link_libraries(kexihandler_migration   kexicore keximigrate )

install(TARGETS kexihandler_migration  DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
