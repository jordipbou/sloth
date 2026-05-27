cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

add_subdirectory(libs/cpnbi)

add_library(sloth_lib INTERFACE)
target_include_directories(sloth_lib INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/
)

# terminal SLOTH REPL

set(SLOTH_FLOATING_POINT ON)

add_executable(sloth repl.c sloth.c)

target_compile_definitions(sloth PRIVATE
	ROOT_PATH="${PROJECT_BINARY_DIR}/../../../../"
	SLOTH_FLOATING_POINT
)

target_link_libraries(sloth cpnbi)

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
target_link_libraries(test_sloth unity)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_EMIT)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_KEY)

if(SLOTH_FLOATING_POINT)
  target_compile_definitions(test_sloth PRIVATE SLOTH_FLOATING_POINT)

 	add_executable(test_sloth_fp tests/test_sloth_fp.c sloth.c)
	target_link_libraries(test_sloth_fp unity)
  target_compile_definitions(test_sloth_fp PRIVATE SLOTH_FLOATING_POINT)

	if(UNIX)
		target_link_libraries(test_sloth m)
    target_link_libraries(test_sloth_fp m)
  endif()
endif()


