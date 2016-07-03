#
# Fetching global Breeze icons Qt resource from KDE git into ${CMAKE_CURRENT_BINARY_DIR}
#

find_package(Git REQUIRED)

#message(STATUS " _RCC_MD5_FILE==${_RCC_MD5_FILE}")
#message(STATUS " _RCC_FILE==${_RCC_FILE}")
#message(STATUS " CMAKE_CURRENT_BINARY_DIR==${CMAKE_CURRENT_BINARY_DIR}")

set(_GIT_URL git://anongit.kde.org/scratch/staniek/kexi-breeze.git)
get_filename_component(_GIT_PARENT_ROOT ${CMAKE_CURRENT_BINARY_DIR}/.. ABSOLUTE)
set(_GIT_ROOT ${_GIT_PARENT_ROOT}/kexi-breeze)
set(_RCC_PATH ${_GIT_ROOT}/share/kexi/icons/${_RCC_FILE})
#message(STATUS " _RCC_PATH==${_RCC_PATH}")

file(READ ${_RCC_MD5_FILE} _EXPECTED_MD5)
#message(STATUS " _EXPECTED_MD5==${_EXPECTED_MD5}")

if(EXISTS ${_RCC_PATH})
    file(MD5 ${_RCC_PATH} _MD5)
    #message(STATUS " _MD5==${_MD5}")
else()
    set(_MD5 0)
endif()

if(NOT ${_MD5} EQUAL ${_EXPECTED_MD5})
    # Fetch file
    if(NOT EXISTS "${_GIT_ROOT}")
        execute_process(COMMAND "${GIT_EXECUTABLE}" clone ${_GIT_URL} ${hash}
            WORKING_DIRECTORY "${_GIT_PARENT_ROOT}"
            RESULT_VARIABLE _RESULT
            OUTPUT_VARIABLE _OUTPUT
        )
    else()
        execute_process(COMMAND "${GIT_EXECUTABLE}" pull origin master
            WORKING_DIRECTORY "${_GIT_ROOT}"
            RESULT_VARIABLE _RESULT
            OUTPUT_VARIABLE _OUTPUT
        )
        if(${_RESULT} EQUAL 0)
            execute_process(COMMAND "${GIT_EXECUTABLE}" reset --hard
                WORKING_DIRECTORY "${_GIT_ROOT}"
                RESULT_VARIABLE _RESULT
                OUTPUT_VARIABLE _OUTPUT
            )
        endif()
    endif()
    if(${_RESULT} EQUAL 0 AND EXISTS "${_RCC_PATH}")
        file(MD5 ${_RCC_PATH} _MD5)
        #message(STATUS " _MD5==${_MD5}")
        if (NOT ${_MD5} STREQUAL ${_EXPECTED_MD5})
            message(FATAL " Unexpected contents of fetched file ${_RCC_PATH} (MD5 SUM=${_MD5}).")
        endif()
    else()
        message(FATAL " Fetching ${_RCC_PATH} failed. Result=${_RESULT}, output=${_OUTPUT}")
    endif()
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_RCC_PATH} .)
