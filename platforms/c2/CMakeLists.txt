cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

add_library(sloth_lib INTERFACE)

target_include_directories(sloth_lib INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/
)

# terminal SLOTH REPL

add_executable(sloth repl.c sloth.c)

target_compile_definitions(sloth PRIVATE
	ROOT_PATH="${PROJECT_BINARY_DIR}/../../../../"
)

if(UNIX)
	# Only needed if compiling floating point support (and on Linux!)
	target_link_libraries(sloth m)
endif()

# Unit tests

add_library(unity STATIC tests/unity.c)
target_include_directories(unity PUBLIC . tests/)
add_executable(test_sloth tests/test_sloth.c sloth.c)
target_link_libraries(test_sloth unity)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_EMIT)
