cmake_minimum_required(VERSION 2.8.0)

find_package(Qt4 REQUIRED)

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

qt4_automoc(${KexiMobile_SRCS})

kexi_add_executable(keximobile ${KexiMobile_SRCS})

target_link_libraries(keximobile Qt5::Core Qt5::Gui
    kexiextendedwidgets
    kexicore
    kdeui
)

install(TARGETS keximobile ${INSTALL_TARGETS_DEFAULT_ARGS})
