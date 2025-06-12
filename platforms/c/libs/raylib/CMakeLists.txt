cmake_minimum_required(VERSION 3.21)
project(C-RAYLIB LANGUAGES C)

add_executable(sray sray.c)

add_compile_definitions(ROOT_PATH="${PROJECT_SOURCE_DIR}/")
add_compile_definitions(FORTH_PATH="${PROJECT_SOURCE_DIR}/../../../../4th/")

# The next lines are only valid for Windows
# I don't know if the PRIVATE is needed, or important

if (WIN32)
	target_include_directories (sray PRIVATE raylib-5.5_win64_msvc16/include ${CMAKE_SOURCE_DIR}/../../)
	
	target_link_libraries (sray PRIVATE ${CMAKE_SOURCE_DIR}/raylib-5.5_win64_msvc16/lib/raylib.lib)
	target_link_libraries(sray PRIVATE winmm.lib)
	target_link_libraries(sray PRIVATE OpenGL32.lib)
	target_link_libraries(sray PRIVATE msvcrt.lib)
endif (WIN32)

