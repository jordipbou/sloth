cmake_minimum_required(VERSION 3.16)
project(SLOTH-REPL LANGUAGES C)

add_executable(sloth repl.c)

add_compile_definitions(ROOT_PATH="../../../")

if (WIN32)
	target_link_libraries(sloth PRIVATE msvcrt.lib)
else ()
	target_link_libraries(sloth m)
endif (WIN32)


