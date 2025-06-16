cmake_minimum_required(VERSION 3.16)
project(SLOTH-RAYLIB LANGUAGES C)

add_library(raylib STATIC IMPORTED)
if(WIN32)
	set_target_properties(raylib PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/raylib-5.5_win64_msvc16/lib/raylib.lib"
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/raylib-5.5_win64_msvc16/include"
	)
elseif(UNIX)
	set_target_properties(raylib PROPERTIES
		IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/raylib-5.5_linux_amd64/lib/libraylib.a"
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/raylib-5.5_linux_amd64/include"
	)
endif()

add_library(sloth_raylib INTERFACE)
target_include_directories(sloth_raylib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(sloth_raylib INTERFACE raylib)

if(WIN32)
	target_link_libraries(sloth_raylib INTERFACE winmm.lib)
	target_link_libraries(sloth_raylib INTERFACE OpenGL32.lib)
	target_link_libraries(sloth_raylib INTERFACE msvcrt.lib)
elseif(UNIX)
endif()

