cmake_minimum_required(VERSION 3.16)
project(SLOTH-SDL3 LANGUAGES C)

set(SDL_STATIC ON)
add_subdirectory(SDL)

# sloth_sdl3 library

add_library(sloth_sdl3 STATIC sloth_sdl3.c)

find_package(Threads REQUIRED)
target_link_libraries(sloth_sdl3 PRIVATE SDL3-static Threads::Threads)

target_include_directories(sloth_sdl3
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../..
		${CMAKE_CURRENT_SOURCE_DIR}/SDL/include
		${CMAKE_CURRENT_SOURCE_DIR})
