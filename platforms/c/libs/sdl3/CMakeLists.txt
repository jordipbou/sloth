cmake_minimum_required(VERSION 3.16)
project(SLOTH-SDL3 LANGUAGES C)

set(SDL_STATIC ON)
add_subdirectory(SDL)

# sloth_sdl3 library

add_library(sloth_sdl3 STATIC sloth_sdl3.c)

find_package(Threads REQUIRED)
target_link_libraries(sloth_sdl3 PRIVATE SDL3-static Threads::Threads)

target_include_directories(sloth_sdl3
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/../..
		${CMAKE_CURRENT_SOURCE_DIR}/SDL3/include
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR})

# sdl3 based sloth executable

if(UNIX)
	add_executable(sdl3_sloth sdl3_repl.c)
	target_link_libraries(sdl3_sloth m pthread)
else()
	# add_executable(sdl3_sloth WIN32 sdl3_repl.c)
	add_executable(sdl3_sloth sdl3_repl.c)
endif()
	
add_compile_definitions(
	ROOT_PATH="${PROJECT_SOURCE_DIR}/../../../../")

target_include_directories(sdl3_sloth PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/SDL/include/)

target_link_libraries(sdl3_sloth sloth_sdl3)
