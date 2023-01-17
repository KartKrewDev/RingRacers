CPMAddPackage(
	NAME imgui
	VERSION 1.89.2
	URL "https://github.com/ocornut/imgui/archive/refs/tags/v1.89.2.zip"
	EXCLUDE_FROM_ALL ON
)
if(imgui_ADDED)
	set(imgui_SOURCES
		imgui.cpp
		imgui.h
		imgui_demo.cpp
		imgui_draw.cpp
		imgui_internal.h
		imgui_tables.cpp
		imgui_widgets.cpp
		imstb_rectpack.h
		imstb_textedit.h
		imstb_truetype.h
	)
	list(TRANSFORM imgui_SOURCES PREPEND "${imgui_SOURCE_DIR}/")

	add_custom_command(
		OUTPUT "${imgui_BINARY_DIR}/include/imgui.h" "${imgui_BINARY_DIR}/include/imconfig.h"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${imgui_BINARY_DIR}/include"
		COMMAND ${CMAKE_COMMAND} -E copy "${imgui_SOURCE_DIR}/imgui.h" "${imgui_SOURCE_DIR}/imconfig.h" "${imgui_BINARY_DIR}/include"
		DEPENDS "${imgui_SOURCE_DIR}/imgui.h" "${imgui_SOURCE_DIR}/imconfig.h"
		VERBATIM
	)
	list(APPEND imgui_SOURCES "${imgui_BINARY_DIR}/include/imgui.h" "${imgui_BINARY_DIR}/include/imconfig.h" "${CMAKE_CURRENT_SOURCE_DIR}/imgui_config/srb2_imconfig.h")
	add_library(imgui STATIC ${imgui_SOURCES})
	target_include_directories(imgui PUBLIC "${imgui_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/imgui_config")
	target_compile_definitions(imgui PUBLIC IMGUI_USER_CONFIG="srb2_imconfig.h")
	target_compile_features(imgui PUBLIC cxx_std_11)
	target_link_libraries(imgui PRIVATE stb_rect_pack)
	add_library(imgui::imgui ALIAS imgui)
endif()
