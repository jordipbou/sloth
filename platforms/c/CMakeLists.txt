cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

add_subdirectory(libs/facility)
add_subdirectory(libs/locals)
add_subdirectory(libs/memory)
add_subdirectory(libs/sqlite)
# add_subdirectory(libs/raylib)
add_subdirectory(libs/plibsys)
# add_subdirectory(libs/systray)
# add_subdirectory(libs/osdialog)
# add_subdirectory(libs/geninput)
add_subdirectory(libs/sdl3)

add_compile_definitions(ROOT_PATH="${PROJECT_SOURCE_DIR}/../../")

add_executable(sloth repl.c)

if(UNIX)
	# Only needed if compiling floating point support (and on Linux!)
	target_link_libraries(sloth m)
endif()

target_link_libraries(sloth sloth_facility_wordset)
target_link_libraries(sloth sloth_locals_wordset)
target_link_libraries(sloth sloth_memory_wordset)
target_link_libraries(sloth sloth_sqlite)
# target_link_libraries(sloth sloth_raylib)
target_link_libraries(sloth sloth_plibsys)
# target_link_libraries(sloth sloth_systray)
# target_link_libraries(sloth sloth_osdialog)
# target_link_libraries(sloth sloth_geninput)

add_executable(gui_sloth gui_repl.c) # WIN32 gui_repl.c)
	
target_include_directories(gui_sloth PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/libs/sdl3/SDL/include/)

target_link_libraries(gui_sloth sloth_facility_wordset)
target_link_libraries(gui_sloth sloth_locals_wordset)
target_link_libraries(gui_sloth sloth_memory_wordset)
target_link_libraries(gui_sloth sloth_sqlite)
# target_link_libraries(gui_sloth sloth_raylib)
target_link_libraries(gui_sloth sloth_plibsys)
# target_link_libraries(gui_sloth sloth_systray)
# target_link_libraries(gui_sloth sloth_osdialog)
# target_link_libraries(gui_sloth sloth_geninput)
target_link_libraries(gui_sloth sloth_sdl3)

