cmake_minimum_required (VERSION 3.8)
project(sloth LANGUAGES C)

set (sloth_VERSION_MAJOR 0)
set (sloth_VERSION_MINOR 1)

if (NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE Release)
endif()

set (CMAKE_C_FLAGS_RELEASE "-O3")
set (CMAKE_EXE_LIMKER_FLAGS "")

add_executable (
	sloth 
	src/sloth)
target_include_directories (sloth PRIVATE src deps/capybara/src)

set_property (TARGET sloth PROPERTY C_STANDARD 99)
