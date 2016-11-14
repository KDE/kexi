include_directories(${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_SOURCE_DIR}/src/core)


########### next target ###############

set(kexidataviewcommon_LIB_SRCS
   kexidataawareobjectiface.cpp
   kexidataawarepropertyset.cpp
   kexidataawareview.cpp
   kexidataprovider.cpp
   kexiformdataiteminterface.cpp
)

kexi_add_library(kexidataviewcommon SHARED ${kexidataviewcommon_LIB_SRCS})

generate_export_header(kexidataviewcommon)

target_link_libraries(
    kexidataviewcommon
    kexicore
    kexiutils
    kexiguiutils

    KDb
    KPropertyWidgets

    KF5::KIOWidgets
    KF5::XmlGui #KActionCollection...
)

install(TARGETS kexidataviewcommon  ${INSTALL_TARGETS_DEFAULT_ARGS})
