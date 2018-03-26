include_directories(
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/kexiutils/style
)

########### next target ###############

set(kexirelationsview_LIB_SRCS
   KexiRelationsScrollArea.cpp
   KexiRelationsConnection.cpp
   KexiRelationsTableContainer.cpp
   KexiRelationsTableContainer_p.cpp
   KexiRelationsView.cpp
)

kexi_add_library(kexirelationsview SHARED ${kexirelationsview_LIB_SRCS})

generate_export_header(kexirelationsview)

target_link_libraries(kexirelationsview
    kexiextendedwidgets
)

install(TARGETS kexirelationsview  ${INSTALL_TARGETS_DEFAULT_ARGS})
