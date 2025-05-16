cmake_minimum_required(VERSION 3.16)
project(SLOTH-SQLITE-REPL LANGUAGES C)

add_compile_definitions(ROOT_PATH="${PROJECT_SOURCE_DIR}/../../../../")

if (WIN32)
	# Avoids use of pthreads on Windows
	add_compile_definitions(SQLITE_THREADSAFE=0)
endif (WIN32)

add_executable(sloth-sqlite repl.c sqlite-amalgamation-3490200/sqlite3.c)

include_directories(../.. sqlite-amalgamation-3490200)

if (WIN32)
	target_link_libraries(sloth-sqlite winmm)
else (WIN32)
	target_link_libraries(sloth-sqlite m)
	target_link_libraries(sloth-sqlite pthread)
	target_link_libraries(sloth-sqlite dl)
endif (WIN32)

