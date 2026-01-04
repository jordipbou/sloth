cmake_minimum_required(VERSION 3.16)
project(sloth LANGUAGES C)

option(DISTRIBUTION_BUILD "Build copying 4th files" ON)

add_subdirectory(platforms/c term)
add_subdirectory(platforms/c/libs/sdl3 gui)

add_custom_target(copy_4th_files_for_c_target ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/4th
		${PROJECT_BINARY_DIR}/term/4th
	COMMENT "Copying 4th files"
)

add_custom_target(copy_4th_files_for_sdl3_target ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${PROJECT_SOURCE_DIR}/4th
		${PROJECT_BINARY_DIR}/gui/4th
	COMMENT "Copying 4th files"
)
