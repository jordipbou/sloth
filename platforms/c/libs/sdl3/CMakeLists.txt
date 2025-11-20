cmake_minimum_required(VERSION 3.16)
project(SLOTH-SDL3 LANGUAGES C)

set(SDL_STATIC ON)
add_subdirectory(SDL)

add_library(sloth_sdl3 STATIC sloth_sdl3.c)

target_link_libraries(sloth_sdl3 PRIVATE SDL3-static)

target_include_directories(sloth_sdl3
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../..
		${CMAKE_CURRENT_SOURCE_DIR}/SDL3/include
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})


