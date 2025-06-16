cmake_minimum_required(VERSION 3.16)
project(SLOTH-REPL LANGUAGES C)

add_subdirectory(libs/sqlite)
add_subdirectory(libs/raylib)

add_executable(sloth repl.c)

add_compile_definitions(ROOT_PATH="${PROJECT_SOURCE_DIR}/../../")

target_link_libraries(sloth PRIVATE sloth_raylib)
target_link_libraries(sloth PRIVATE sloth_sqlite)
