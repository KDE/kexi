include_directories(${CMAKE_SOURCE_DIR}/src/widget/utils)

set(kexiguiutils_LIB_SRCS
    kexisharedactionclient.cpp
    kexidisplayutils.cpp
    kexitooltip.cpp
    kexicontextmenuutils.cpp
    kexidropdownbutton.cpp
    kexicomboboxdropdownbutton.cpp
    kexidatetimeformatter.cpp
    KexiDockableWidget.cpp
)

if(SHOULD_BUILD_KEXI_DESKTOP_APP)
    list(APPEND kexiguiutils_LIB_SRCS
        kexirecordnavigator.cpp
    )
endif()

kexi_add_library(kexiguiutils SHARED ${kexiguiutils_LIB_SRCS})

generate_export_header(kexiguiutils)

target_link_libraries(kexiguiutils
    kexicore

    KF5::KIOWidgets
)

install(TARGETS kexiguiutils  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
