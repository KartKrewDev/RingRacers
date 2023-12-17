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

add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/imgui.h" "${CMAKE_CURRENT_BINARY_DIR}/include/imconfig.h"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/include"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/imgui.h" "${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h" "${CMAKE_CURRENT_BINARY_DIR}/include"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/imgui.h" "${CMAKE_CURRENT_SOURCE_DIR}/imconfig.h"
	VERBATIM
)
list(APPEND imgui_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/include/imgui.h" "${CMAKE_CURRENT_BINARY_DIR}/include/imconfig.h" "${CMAKE_CURRENT_SOURCE_DIR}/../imgui_config/srb2_imconfig.h")
add_library(imgui STATIC ${imgui_SOURCES})
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../imgui_config")
target_compile_definitions(imgui PUBLIC IMGUI_USER_CONFIG="srb2_imconfig.h")
target_compile_features(imgui PUBLIC cxx_std_11)
target_link_libraries(imgui PRIVATE Stb::Stb)
add_library(imgui::imgui ALIAS imgui)
