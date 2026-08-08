cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

# -- Set default build type --------------------------------

# For single-config generators (Makefiles, Ninja)
# CMAKE_BUILD_TYPE must be set for the Debug/Release
# ROOT_PATH selection below to work; default to Release if the
# user didn't choose one explicitly.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE
            "Release"
            CACHE STRING "Build type" FORCE)
endif()

# For multi-config generators (Visual Studio, Ninja Multi-Config)
# default to Release too: put Release first in
# CMAKE_CONFIGURATION_TYPES (used as the default by Visual Studio)
# and set CMAKE_DEFAULT_BUILD_TYPE (used by Ninja Multi-Config).
# Debug remains available via --config Debug.
if(CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_DEFAULT_BUILD_TYPE)
    set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Build types" FORCE)
    set(CMAKE_DEFAULT_BUILD_TYPE "Release" CACHE STRING "Default build type" FORCE)
endif()

# Apply strict C90 only to sloth's own targets:
function(sloth_strict_c target)
  set_target_properties(${target} PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
  target_compile_options(${target} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-pedantic-errors -Wall -Wextra -fno-common>)
endfunction()

# Make compilation fail if the standard is not enforced
# (GCC/Clang-specific flag, not portable syntax).
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(-pedantic-errors -Wall -Wextra)
endif()

# Not declaring a global variable as extern in the header
# gives a linker error if the -fcommon flag is not used.
# That flag was changed to -fno-common as default in GCC 10.
# As my current GCC is older than that, it uses -fcommon by
# default and does not report the error.
# Enforcing -fno-common to all builds to ensure this type
# of errors do not pass thru again.
add_compile_options(-fno-common)

# -- Required flags for Windows builds --------------------

# Must be before FetchContent to ensure CPNBI and Unity
# also use the correct flags on Windows.

if (WIN32)
	add_compile_definitions(	
		_CRT_SECURE_NO_WARNINGS 
		_CRT_NONSTDC_NO_DEPRECATE)
 
	set(
		CMAKE_MSVC_RUNTIME_LIBRARY 
		"MultiThreaded$<$<CONFIG:Debug>:Debug>" 
		CACHE STRING "" FORCE)
 
	add_compile_definitions(_MT)
endif()

# -- Fetch all external dependencies ----------------------

include(FetchContent)

# ThrowTheSwitch/Unity is the test suite used by Sloth
FetchContent_Declare(
	unity
	GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
	GIT_TAG bbf8f3728a937c7627b8094de7ae13559d220ed5	
)

FetchContent_MakeAvailable(unity)

target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
if(UNIX)
    target_link_libraries(unity m)
endif()

add_library(sloth_lib STATIC sloth.c)
target_include_directories(
	sloth_lib
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
				 $<INSTALL_INTERFACE:>)
set_target_properties(
	sloth_lib
	PROPERTIES C_STANDARD 90
						 C_STANDARD_REQUIRED OFF
						 C_EXTENSIONS OFF)

if(DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	target_compile_definitions(
		sloth_lib PRIVATE SLOTH_WITHOUT_FLOATING_POINT)
endif()

# The optional FILE word set

if(NOT DEFINED SLOTH_WITHOUT_FILE_WORD_SET)
	add_library(sloth_file_word_set STATIC file.c)
	target_include_directories(
		sloth_file_word_set
		PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
					 $<INSTALL_INTERFACE:>)
	set_target_properties(
		sloth_file_word_set
		PROPERTIES C_STANDARD 90
							 C_STANDARD_REQUIRED OFF
							 C_EXTENSIONS OFF)

	target_link_libraries(sloth_lib sloth_file_word_set)
endif()

if(NOT DEFINED SLOTH_WITHOUT_MEMORY_WORD_SET)
	add_library(sloth_memory_word_set STATIC memory.c)
	target_include_directories(
		sloth_memory_word_set
		PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
					 $<INSTALL_INTERFACE:>)
	set_target_properties(
		sloth_memory_word_set
		PROPERTIES C_STANDARD 90
							 C_STANDARD_REQUIRED OFF
							 C_EXTENSIONS OFF)

	target_link_libraries(sloth_lib sloth_memory_word_set)
endif()

# terminal SLOTH REPL

add_executable(sloth repl.c)

# Debug builds read 4th/ straight from the source tree, so
# editing the Forth sources doesn't require a rebuild.
# Release builds read 4th/ from next to the executable,
# where it gets copied to below. This depends on CONFIG, not
# on platform, so it works the same way on Linux and Windows
# for both single- and multi-config generators.
target_compile_definitions(
	sloth 
	PRIVATE
	ROOT_PATH="$<IF:$<CONFIG:Debug>,${CMAKE_CURRENT_SOURCE_DIR}/../../,$<TARGET_FILE_DIR:sloth>/>"
)

# Make sure 4th/ is available next to the executable for 
# release (and any non-Debug) builds. Debug builds read 
# straight from the source tree (see ROOT_PATH above), 
# so this copy is skipped there.
add_custom_command(TARGET sloth POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,echo,copy_directory>
	$<$<NOT:$<CONFIG:Debug>>:${CMAKE_CURRENT_SOURCE_DIR}/../../4th>
		$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE_DIR:sloth>/4th>
	COMMENT "Copying 4th/ next to the sloth executable (release only)"
)

if(DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	target_compile_definitions(
		sloth PRIVATE SLOTH_WITHOUT_FLOATING_POINT)
endif()

if(DEFINED SLOTH_WITHOUT_FILE_WORD_SET)
	target_compile_definitions(
		sloth PRIVATE SLOTH_WITHOUT_FILE_WORD_SET)
endif()

if(DEFINED SLOTH_WITHOUT_MEMORY_WORD_SET)
	target_compile_definitions(
		sloth PRIVATE SLOTH_WITHOUT_MEMORY_WORD_SET)
endif()

target_link_libraries(sloth sloth_lib)

# Unit tests

add_executable(test_sloth tests/test_sloth.c sloth.c)
target_link_libraries(test_sloth sloth_lib unity::framework)

if(NOT DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	add_executable(test_sloth_fp tests/test_sloth_fp.c
															 sloth.c)
	target_link_libraries(test_sloth_fp sloth_lib
												unity::framework)
	if(UNIX)
		target_link_libraries(sloth_lib m)
		target_link_libraries(sloth m)
		target_link_libraries(test_sloth m)
		target_link_libraries(test_sloth_fp m)
	endif()
endif()

# Apply strict C90 to sloth's targets
sloth_strict_c(sloth)
sloth_strict_c(sloth_lib)
sloth_strict_c(sloth_memory_word_set)
sloth_strict_c(sloth_file_word_set)
