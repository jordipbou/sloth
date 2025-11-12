cmake_minimum_required(VERSION 3.16)
project(SLOTH-LOCALS-WORDSET LANGUAGES C)

add_library(sloth_locals_wordset STATIC locals.c)

target_include_directories(sloth_locals_wordset PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
