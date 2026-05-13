cmake_minimum_required(VERSION 3.16)
project(CPNBI LANGUAGES C)

add_library(cpnbi STATIC cpnbi.c)

target_include_directories(cpnbi PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
