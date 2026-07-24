cmake_minimum_required(VERSION 3.20)

project(playbacker LANGUAGES C CXX)

#
# Standards
#

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Répertoires de sortie
#

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_BINARY_DIR}/../bin
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    ${CMAKE_BINARY_DIR}/../bin/Debug
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    ${CMAKE_BINARY_DIR}/../bin/Release
)


#
# Build par défaut : Debug
#

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

#
# Warnings
#

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
endif()


if(WIN32)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
    )
endif()

#
# Sources de la bibliothèque commune
#

file(GLOB_RECURSE PBKR_LIB_SOURCES
    CONFIGURE_DEPENDS
    "../_dev/src/pbkr_lib/*.c"
    "../_dev/src/pbkr_lib/*.cpp"
)

add_library(pbkr_lib STATIC
    ${PBKR_LIB_SOURCES}
)

target_include_directories(pbkr_lib
    PUBLIC
        ../_dev/src/pbkr_lib
)

#
# Un exécutable par source dans mains/
#

file(GLOB MAIN_SOURCES
    CONFIGURE_DEPENDS
    "../_dev/src/mains/*.cpp"
)

foreach(MAIN_FILE ${MAIN_SOURCES})


    get_filename_component(EXE_NAME ${MAIN_FILE} NAME_WE)
    Message("Building ${EXE_NAME}")

    add_executable(${EXE_NAME}
        ${MAIN_FILE}
    )

    target_link_libraries(${EXE_NAME}
        PRIVATE
            pbkr_lib
    )
        

endforeach()