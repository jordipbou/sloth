cmake_minimum_required(VERSION 3.16)
project(SLOTH-SQLITE LANGUAGES C)

add_library(sqlite3 STATIC ${CMAKE_CURRENT_SOURCE_DIR}/sqlite-amalgamation-3490200/sqlite3.c)
target_include_directories(sqlite3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/sqlite-amalgamation-3490200")

if(WIN32)
	# Avoids use of pthreads on Windows
	target_compile_definitions(sqlite3 PRIVATE SQLITE_THREADSAFE=0)
	target_link_libraries(sqlite3 PRIVATE winmm)
elseif(UNIX)
	target_link_libraries(sqlite3 PRIVATE m pthread dl)
endif()

add_library(sloth_sqlite INTERFACE)
target_include_directories(sloth_sqlite INTERFACE	
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(sloth_sqlite INTERFACE sqlite3)
