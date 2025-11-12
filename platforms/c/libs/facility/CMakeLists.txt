cmake_minimum_required(VERSION 3.16)
project(SLOTH-FACILITY-WORDSET LANGUAGES C)

add_library(sloth_facility_wordset STATIC facility.c cpnbi.c)

target_include_directories(sloth_facility_wordset PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)
