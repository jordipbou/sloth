cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

add_subdirectory(libs/facility)
add_subdirectory(libs/file)
add_subdirectory(libs/locals)
add_subdirectory(libs/memory)
# add_subdirectory(libs/sqlite)
# add_subdirectory(libs/plibsys)
# add_subdirectory(libs/systray)
# add_subdirectory(libs/osdialog)
add_subdirectory(libs/geninput)
add_subdirectory(libs/sdl3)

if(DISTRIBUTION_BUILD)
	add_compile_definitions(ROOT_PATH="${PROJECT_BINARY_DIR}/")
else()
	add_compile_definitions(ROOT_PATH="${PROJECT_SOURCE_DIR}/../../")
endif()

add_library(sloth_lib INTERFACE)

target_include_directories(sloth_lib INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/
)

# terminal SLOTH REPL

add_executable(sloth repl.c)

if(UNIX)
	# Only needed if compiling floating point support (and on Linux!)
	target_link_libraries(sloth m)
endif()

target_link_libraries(sloth sloth_facility_wordset)
target_link_libraries(sloth sloth_file_wordset)
target_link_libraries(sloth sloth_locals_wordset)
target_link_libraries(sloth sloth_memory_wordset)
# target_link_libraries(sloth sloth_sqlite)
# target_link_libraries(sloth sloth_plibsys)
# target_link_libraries(sloth sloth_systray)
# target_link_libraries(sloth sloth_osdialog)
# target_link_libraries(sloth sloth_geninput)

# SDL3 based SLOTH executable

if(UNIX)
	add_executable(sdl3_sloth sdl3_repl.c)
	target_link_libraries(sdl3_sloth m pthread)
else()
	# add_executable(sdl3_sloth WIN32 sdl3_repl.c)
	add_executable(sdl3_sloth sdl3_repl.c)
endif()
	
target_link_libraries(sdl3_sloth sloth_sdl3)
target_link_libraries(sdl3_sloth sloth_facility_wordset)
target_link_libraries(sdl3_sloth sloth_file_wordset)
target_link_libraries(sdl3_sloth sloth_locals_wordset)
target_link_libraries(sdl3_sloth sloth_memory_wordset)

