# Builds and executes runtime check for existence of Breeze icons

try_run(RUN_RESULT COMPILE_OK
    ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp
    ${CMAKE_SOURCE_DIR}/cmake/modules/CheckGlobalBreezeIcons.cpp
    LINK_LIBRARIES Qt5::Widgets
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${CMAKE_SOURCE_DIR}/src/main"
    COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
    RUN_OUTPUT_VARIABLE RUN_OUTPUT)

if(NOT COMPILE_OK)
    message(FATAL_ERROR "${COMPILE_OUTPUT}")
endif()

if(NOT ${RUN_RESULT} EQUAL 0)
    message(FATAL_ERROR "No valid breeze-icons.rcc resource file found. \
        The CheckGlobalBreezeIcons program returned ${RUN_RESULT}. \
        Result: ${RUN_OUTPUT}")
endif()
