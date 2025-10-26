cmake_minimum_required(VERSION 3.16)
project(SLOTH-MEMORY-WORDSET LANGUAGES C)

add_library(sloth-memory-wordset STATIC memory.c)

target_include_directories(sloth-memory-wordset PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
