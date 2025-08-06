cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

add_subdirectory(libs/facility)
add_subdirectory(libs/sqlite)
add_subdirectory(libs/raylib)
add_subdirectory(libs/plibsys)

add_executable(sloth repl.c)

add_compile_definitions(ROOT_PATH="${PROJECT_SOURCE_DIR}/../../")

if(WIN32)
elseif(UNIX)
	# Only needed if compiling floating point support (and on Linux!)
	target_link_libraries(sloth m)
endif()

target_link_libraries(sloth sloth-facility-wordset)
target_link_libraries(sloth sloth_sqlite)
target_link_libraries(sloth sloth_raylib)
target_link_libraries(sloth sloth_plibsys)
