cmake_minimum_required(VERSION 3.12)
project(sloth_systray LANGUAGES C)

add_library(systray STATIC systray.c)
add_library(sloth_systray STATIC sloth_systray.c)
 
target_include_directories(sloth_systray PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/../../
)

# Only build on Windows for now
if (WIN32)
    target_link_libraries(sloth_systray
        PUBLIC
            user32
            shell32
						gdi32
						systray
    )
else()
    message(WARNING "systray currently only supports Windows.")
endif()
