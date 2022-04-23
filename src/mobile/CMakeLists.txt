cmake_minimum_required(VERSION 2.8.0)

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core Gui)

include_directories( ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/widget
)

set(KexiMobile_SRCS main.cpp
    KexiMobileMainWindow.cpp
    KexiMobileWidget.cpp
    KexiMobileNavigator.cpp
    KexiMobileToolbar.cpp
)

kexi_add_executable(keximobile ${KexiMobile_SRCS})

target_link_libraries(keximobile
    kexiextendedwidgets
    kexicore
)

install(TARGETS keximobile ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
