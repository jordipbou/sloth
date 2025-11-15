cmake_minimum_required(VERSION 3.16)
project(SLOTH-OSDIALOG LANGUAGES C)

if(WIN32)
	add_library(osdialog STATIC 
		osdialog/osdialog.c 
		osdialog/osdialog_win.c)

	target_link_libraries(osdialog PRIVATE comdlg32)

	if (MSVC)
    target_compile_definitions(osdialog PRIVATE 
			snwprintf=_snwprintf)
	endif()

elseif(UNIX)
	add_library(osdialog STATIC 
		osdialog/osdialog.c 
		osdialog/osdialog_gtk.c)
endif()

add_library(sloth_osdialog STATIC sloth_osdialog.c)

target_include_directories(sloth_osdialog PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/../../
	${CMAKE_CURRENT_SOURCE_DIR}/
	${CMAKE_CURRENT_SOURCE_DIR}/osdialog/)

target_link_libraries(sloth_osdialog PRIVATE osdialog)
