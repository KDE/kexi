set(kexiundo_LIB_SRCS
	kundo2stack.cpp
	kundo2group.cpp
	kundo2view.cpp
	kundo2model.cpp
	kundo2magicstring.cpp
	kundo2commandextradata.cpp
)

kexi_add_library(kexiundo SHARED ${kexiundo_LIB_SRCS})
generate_export_header(kexiundo BASE_NAME kexiundo)

target_link_libraries(kexiundo
    PUBLIC
        KF5::I18n
        KF5::XmlGui
        Qt5::Widgets
    PRIVATE
        KF5::ConfigWidgets
        KF5::IconThemes #KIconLoader
        kexicore # TODO remove when KexiIcon() goes out of core/
)

install(TARGETS kexiundo ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if(FALSE) # no need to install
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kexiundo_export.h
    kundo2stack.h
    kundo2qstack.h
    kundo2group.h
    kundo2view.h
    kundo2model.h
    kundo2command.h
    kundo2magicstring.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/kexi COMPONENT Devel
)
endif()
