cmake_minimum_required(VERSION 3.16)
project(SLOTH-MEMORY-WORDSET LANGUAGES C)

add_library(sloth_memory_wordset STATIC memory.c)

target_include_directories(sloth_memory_wordset PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
