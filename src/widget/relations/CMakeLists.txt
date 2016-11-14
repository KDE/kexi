include_directories(${CMAKE_SOURCE_DIR}/src/core)

########### next target ###############

set(kexirelationsview_LIB_SRCS
   KexiRelationsScrollArea.cpp
   KexiRelationsConnection.cpp
   KexiRelationsTableContainer.cpp
   KexiRelationsTableContainer_p.cpp
   KexiRelationsView.cpp
)


add_library(kexirelationsview SHARED ${kexirelationsview_LIB_SRCS})

generate_export_header(kexirelationsview)

target_link_libraries(
    kexirelationsview
    kexicore
    kexiextendedwidgets
    kexiutils

    KDb

    Qt5::Core
    Qt5::Gui
    )

set_target_properties(kexirelationsview PROPERTIES VERSION ${GENERIC_PROJECT_LIB_VERSION} SOVERSION ${GENERIC_PROJECT_LIB_SOVERSION} )
install(TARGETS kexirelationsview  ${INSTALL_TARGETS_DEFAULT_ARGS})
