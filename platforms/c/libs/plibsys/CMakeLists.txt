cmake_minimum_required(VERSION 3.16)
project(sloth-plibsys LANGUAGES C)

# We don't want a DLL for plibsys, only the static library.
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(plibsys-0.0.5)

add_library(sloth_plibsys STATIC sloth_plibsys.c)

target_include_directories(sloth_plibsys
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/plibsys-0.0.5/src
		${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(sloth_plibsys PRIVATE plibsysstatic)

target_include_directories(sloth_plibsys PUBLIC
  $<TARGET_PROPERTY:plibsys,INTERFACE_INCLUDE_DIRECTORIES>
)

