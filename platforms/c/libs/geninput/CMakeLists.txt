cmake_minimum_required(VERSION 3.16)
project(SLOTH-GENINPUT LANGUAGES C)

add_library(sloth_geninput STATIC sloth_geninput.c)

target_include_directories(sloth_geninput 
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../sdl3/SDL/include/
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../../
		${CMAKE_CURRENT_SOURCE_DIR}/)
