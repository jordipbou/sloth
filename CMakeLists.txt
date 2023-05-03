cmake_minimum_required (VERSION 3.8)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-ansi -pedantic -pg")

project(sloth LANGUAGES C)

set(sloth_VERSION_MAJOR 0)
set(sloth_VERSION_MINOR 1)

add_executable (
	sloth 
	src/sloth)

target_include_directories (sloth PRIVATE src)
