cmake_minimum_required(VERSION 3.16)
project(SLOTH-LOCALS-WORDSET LANGUAGES C)

add_library(sloth-locals-wordset STATIC locals.c)

target_include_directories(sloth-locals-wordset PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
