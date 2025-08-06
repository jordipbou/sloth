cmake_minimum_required(VERSION 3.16)
project(SLOTH-FACILITY-WORDSET LANGUAGES C)

add_library(sloth-facility-wordset STATIC facility.c cpnbi.c)

target_include_directories(sloth-facility-wordset PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
