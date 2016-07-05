#
# Fetching global Breeze icons Qt resource from KDE git into ${CMAKE_CURRENT_BINARY_DIR}
#

find_package(Git REQUIRED)

#message(STATUS " _RCC_VERSION==${_RCC_VERSION}")
#message(STATUS " _RCC_FILE==${_RCC_FILE}")
#message(STATUS " CMAKE_CURRENT_BINARY_DIR==${CMAKE_CURRENT_BINARY_DIR}")

set(_GIT_URL git://anongit.kde.org/scratch/staniek/kexi-files.git)

get_filename_component(_GIT_PARENT_ROOT ${CMAKE_CURRENT_BINARY_DIR}/.. ABSOLUTE)
set(_GIT_ROOT ${_GIT_PARENT_ROOT}/kexi-files)
set(_RCC_GIT_PATH ${_GIT_ROOT}/${_RCC_VERSION}/share/kexi/icons/${_RCC_FILE})
#message(STATUS " _RCC_GIT_PATH==${_RCC_GIT_PATH}")
set(_RCC_PATH ${CMAKE_CURRENT_BINARY_DIR}/${_RCC_FILE})
#message(STATUS " _RCC_PATH==${_RCC_PATH}")

if(NOT EXISTS ${_RCC_PATH})
    # Fetch file
    if(NOT EXISTS ${_RCC_GIT_PATH})
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
    endif()
    # Copy fetch file
    if(EXISTS ${_RCC_GIT_PATH})
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_RCC_GIT_PATH} ${_RCC_PATH})
    else()
        message(FATAL " Fetching ${_RCC_GIT_PATH} failed. Result=${_RESULT}, output=${_OUTPUT}")
    endif()
endif()
