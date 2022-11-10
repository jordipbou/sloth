cmake_minimum_required (VERSION 3.8)
project(sloth LANGUAGES C)

set (sloth_VERSION_MAJOR 0)
set (sloth_VERSION_MINOR 1)

if (NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE Release)
endif()

set (CMAKE_C_FLAGS_RELEASE "-O3")
set (CMAKE_EXE_LIMKER_FLAGS "")

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)

add_executable (
	sloth 
	src/sloth)
target_include_directories (sloth PRIVATE src deps/tinycc)

add_custom_command (
	TARGET sloth POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
				  ${CMAKE_SOURCE_DIR}/deps/tinycc/libtcc1.a
					${CMAKE_CURRENT_BINARY_DIR}/libtcc1.a)

target_link_directories (sloth PRIVATE deps/tinycc)
target_link_libraries (sloth PRIVATE tcc Threads::Threads ${CMAKE_DL_LIBS})

set_property (TARGET sloth PROPERTY C_STANDARD 99)
