cmake_minimum_required(VERSION 3.17)
project(sloth-repl LANGUAGES C)

enable_testing()

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
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	add_compile_options(-fno-common)
endif()

# -- Required flags for Windows builds --------------------

# Must be before FetchContent to ensure CPNBI and Unity
# also use the correct flags on Windows.

if (WIN32)
	add_compile_definitions(	
		_CRT_SECURE_NO_WARNINGS 
		_CRT_NONSTDC_NO_DEPRECATE)

	if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
		add_compile_definitions(_AMD64_)
	endif()

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
	$<$<CONFIG:Debug>:ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../">
	$<$<CONFIG:Release>:ROOT_PATH="">
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

# The Forth 2012 suite is also read from next to the executable
# in non-Debug builds (Debug reads it straight from the source
# tree via ROOT_PATH), so `ctest` finds it in both configs.
add_custom_command(TARGET sloth POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,echo,copy_directory>
	$<$<NOT:$<CONFIG:Debug>>:${CMAKE_CURRENT_SOURCE_DIR}/../../forth2012-test-suite>
		$<$<NOT:$<CONFIG:Debug>>:$<TARGET_FILE_DIR:sloth>/forth2012-test-suite>
	COMMENT "Copying forth2012-test-suite/ next to the sloth executable (release only)"
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

# -- CTest -------------------------------------------------
# Single entry point for all C tests, on Linux and Windows:
#   ctest --test-dir build -C <config> --output-on-failure
# The Forth suite runs next to the executable so relative paths
# resolve in every config.

add_test(NAME sloth_unit COMMAND test_sloth)

if(NOT DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	add_test(NAME sloth_unit_fp COMMAND test_sloth_fp)
endif()

# The Forth suite writes binary data, so it is checked by a small
# CMake script that counts the reported failures (see the script for
# the baseline). Without the FP word set only the ACCEPT failure
# remains.
if(DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	set(SLOTH_FORTH_BASELINE 1)
else()
	set(SLOTH_FORTH_BASELINE 2)
endif()

add_test(NAME sloth_forth
	COMMAND ${CMAKE_COMMAND}
		-DSLOTH=$<TARGET_FILE:sloth>
		-DWORKDIR=$<TARGET_FILE_DIR:sloth>
		-DBASELINE=${SLOTH_FORTH_BASELINE}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/check_forth_tests.cmake)
