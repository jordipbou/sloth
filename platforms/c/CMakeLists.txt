cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

# Check the SLOTH_FLOATING_POINT thing, I don't like
# how it's done.
# Also, I think it should be ON by default, not the 
# opposite.

add_subdirectory(libs/cpnbi)

add_library(sloth_lib STATIC sloth.c)
target_include_directories(sloth_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<INSTALL_INTERFACE:>  # for install(EXPORT ...) scenarios
)
if (SLOTH_FLOATING_POINT)
	target_compile_definitions(sloth_lib PRIVATE SLOTH_FLOATING_POINT)
endif()

# terminal SLOTH REPL

set(SLOTH_FLOATING_POINT ON)

add_executable(sloth repl.c)

if(NOT DEFINED SLOTH_ROOT_PATH)
	set(SLOTH_ROOT_PATH "${PROJECT_BINARY_DIR}/../../../../")
endif()

target_compile_definitions(sloth PRIVATE
	ROOT_PATH="${SLOTH_ROOT_PATH}"
	SLOTH_FLOATING_POINT
)

target_link_libraries(sloth sloth_lib cpnbi_lib)

if(SLOTH_FLOATING_POINT)
    target_compile_definitions(sloth PRIVATE SLOTH_FLOATING_POINT)
    if(UNIX)
        target_link_libraries(sloth m)
    endif()
endif()

# Unit tests

add_library(unity STATIC tests/unity.c)
target_include_directories(unity PUBLIC . tests/)
if(SLOTH_FLOATING_POINT)
	target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
	if(UNIX)
		target_link_libraries(unity m)
  endif()
endif()

add_executable(test_sloth tests/test_sloth.c sloth.c)
target_link_libraries(test_sloth sloth_lib unity)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_EMIT)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_KEY)

if(SLOTH_FLOATING_POINT)
  target_compile_definitions(test_sloth PRIVATE SLOTH_FLOATING_POINT)

 	add_executable(test_sloth_fp tests/test_sloth_fp.c sloth.c)
	target_link_libraries(test_sloth_fp sloth_lib unity)
  target_compile_definitions(test_sloth_fp PRIVATE SLOTH_FLOATING_POINT)

	if(UNIX)
		target_link_libraries(test_sloth m)
    target_link_libraries(test_sloth_fp m)
  endif()
endif()


