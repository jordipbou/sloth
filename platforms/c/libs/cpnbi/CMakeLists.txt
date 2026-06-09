cmake_minimum_required(VERSION 3.16)
project(CPNBI LANGUAGES C)

add_library(cpnbi_lib STATIC cpnbi.c)

target_include_directories(cpnbi_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<INSTALL_INTERFACE:>  # for install(EXPORT ...) scenarios
)
