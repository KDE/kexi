include_directories(${XBase_INCLUDE_DIR})

########### next target ###############

set(keximigrate_xbase_PART_SRCS
    xbasemigrate.cpp
    Messages.sh
)

add_library(keximigrate_xbase MODULE ${keximigrate_xbase_PART_SRCS})


target_link_libraries(keximigrate_xbase
    keximigrate
    kexiutils

    KDb

    ${XBase_LIBRARIES}
)

install(TARGETS keximigrate_xbase DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
