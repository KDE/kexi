#set(LIBS keximain kexicore kexiextendedwidgets)
#kexi_add_test(kexi GlobalSearchTest ${LIBS})
ecm_add_tests(
    GlobalSearchTest.cpp
    LINK_LIBRARIES
        Qt5::Test
        keximain
        kexiextendedwidgets
)
set_property(TARGET GlobalSearchTest PROPERTY COMPILE_FLAGS
    ${_COMPILE_FLAGS}\ -DCURRENT_SOURCE_DIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/\\""\ -DFILES_OUTPUT_DIR="\\"${CMAKE_CURRENT_BINARY_DIR}/\\"")
