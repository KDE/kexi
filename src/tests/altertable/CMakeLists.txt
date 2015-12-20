

include_directories(${CMAKE_SOURCE_DIR}/src/widget ${CMAKE_SOURCE_DIR}/src/core)

# temp
add_definitions( -DNO_GUI=1 )

set(kexialtertabletest_SRCS main.cpp )


add_executable(kexialtertabletest ${kexialtertabletest_SRCS})

target_link_libraries(kexialtertabletest KDb)

