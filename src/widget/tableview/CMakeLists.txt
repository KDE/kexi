include_directories(${CMAKE_SOURCE_DIR}/src/core)

########### next target ###############

set(kexidatatable_LIB_SRCS
   KexiTableScrollArea.cpp
   KexiTableScrollArea_p.cpp
   KexiTableScrollAreaHeader.cpp
   KexiTableScrollAreaHeaderModel.cpp
   KexiDataTableView.cpp
   KexiDataTableScrollArea.cpp
   KexiTableScrollAreaWidget.cpp
   kexicelleditorfactory.cpp
   kexitableedit.cpp
   kexiinputtableedit.cpp
   kexiblobtableedit.cpp
   kexibooltableedit.cpp
   kexicomboboxbase.cpp
   kexicomboboxtableedit.cpp
   kexicomboboxpopup.cpp
   kexidatetableedit.cpp
   kexitimetableedit.cpp
   kexidatetimetableedit.cpp
   kexitextformatter.cpp
)

kexi_add_library(kexidatatable SHARED ${kexidatatable_LIB_SRCS})

generate_export_header(kexidatatable)

target_link_libraries(kexidatatable
    kexidataviewcommon
    kexiguiutils

    KF5::KIOWidgets
)

install(TARGETS kexidatatable  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
