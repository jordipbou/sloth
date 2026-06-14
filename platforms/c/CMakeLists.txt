cmake_minimum_required(VERSION 3.16)
project(sloth-repl LANGUAGES C)

include(cmake/dependencies.cmake)

add_library(sloth_lib STATIC sloth.c)
target_include_directories(sloth_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<INSTALL_INTERFACE:>
)

if(DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	target_compile_definitions(sloth_lib PRIVATE 
		SLOTH_WITHOUT_FLOATING_POINT)
endif()

# The optional FILE word set

if(NOT DEFINED SLOTH_WITHOUT_FILE_WORD_SET)
	add_library(sloth_file_word_set STATIC file.c)
	target_include_directories(sloth_file_word_set
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
			$<INSTALL_INTERFACE:>
	)

	target_link_libraries(sloth_lib sloth_file_word_set)
endif()

if(NOT DEFINED SLOTH_WITHOUT_MEMORY_WORD_SET)
	add_library(sloth_memory_word_set STATIC memory.c)
	target_include_directories(sloth_memory_word_set
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
			$<INSTALL_INTERFACE:>
	)

	target_link_libraries(sloth_lib sloth_memory_word_set)
endif()

# terminal SLOTH REPL

add_executable(sloth repl.c)
target_compile_definitions(sloth PRIVATE
	ROOT_PATH="${PROJECT_BINARY_DIR}/../../../../"
)

if(DEFINED SLOTH_WITHOUT_FLOATING_POINT)
	target_compile_definitions(sloth PRIVATE
		SLOTH_WITHOUT_FLOATING_POINT
	)
endif()

if(DEFINED SLOTH_WITHOUT_FILE_WORD_SET)
	target_compile_definitions(sloth PRIVATE
		SLOTH_WITHOUT_FILE_WORD_SET
	)
endif()

if(DEFINED SLOTH_WITHOUT_MEMORY_WORD_SET)
	target_compile_definitions(sloth PRIVATE
		SLOTH_WITHOUT_MEMORY_WORD_SET
	)
endif()

target_link_libraries(sloth sloth_lib cpnbi_lib)

# Unit tests

add_executable(test_sloth tests/test_sloth.c sloth.c)
target_link_libraries(test_sloth sloth_lib unity::framework)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_EMIT)
target_compile_definitions(test_sloth PRIVATE SLOTH_CUSTOM_KEY)

if(NOT DEFINED SLOTH_WITHOUT_FLOATING_POINT)
 	add_executable(test_sloth_fp tests/test_sloth_fp.c sloth.c)
	target_link_libraries(test_sloth_fp sloth_lib unity::framework)
	if(UNIX)
		target_link_libraries(sloth_lib m)
		target_link_libraries(sloth m)
		target_link_libraries(test_sloth m)
    target_link_libraries(test_sloth_fp m)
  endif()
endif()


