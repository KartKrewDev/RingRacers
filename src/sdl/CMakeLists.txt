# Declare SDL3 interface sources

target_sources(SRB2SDL2 PRIVATE
	new_sound.cpp
	ogl_sdl.c
	rhi_gl2_platform.cpp
	rhi_gl2_platform.hpp
	i_threads.c
	i_net.c
	i_system.cpp
	i_main.cpp
	i_video.cpp
	IMG_xpm.c
	endtxt.c
	hwsym_sdl.c
)

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	target_sources(SRB2SDL2 PRIVATE
		../win32/win_dbg.c
		../win32/Srb2win.rc)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
	set_source_files_properties(macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	target_sources(SRB2SDL2 PRIVATE
		macosx/mac_alert.c
		macosx/mac_alert.h
		macosx/mac_resources.c
		macosx/mac_resources.h
		macosx/Srb2mac.icns
	)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	find_library(CORE_FOUNDATION_LIBRARY "CoreFoundation")
	target_link_libraries(SRB2SDL2 PRIVATE
		${CORE_FOUNDATION_LIBRARY}
	)

	# Configure the app bundle icon and plist properties
	target_sources(SRB2SDL2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Srb2mac.icns")
	set_target_properties(SRB2SDL2 PROPERTIES
		MACOSX_BUNDLE_ICON_FILE "Srb2mac"
		MACOSX_BUNDLE_BUNDLE_NAME "Dr. Robotnik's Ring Racers"
		MACOSX_BUNDLE_BUNDLE_VERSION ${SRB2_VERSION}

		RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Srb2mac.icns"
	)
endif()

target_link_libraries(SRB2SDL2
	PRIVATE
	$<IF:$<TARGET_EXISTS:SDL3::SDL3>,SDL3::SDL3,SDL3::SDL3-static>
)

if("${CMAKE_SYSTEM_NAME}" MATCHES Linux)
	target_link_libraries(SRB2SDL2 PRIVATE m rt)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES Windows)
	target_link_libraries(SRB2SDL2 PRIVATE
		ws2_32
	)
	# target_compile_options(SRB2SDL2 PRIVATE
	# 	-U_WINDOWS
	# )
endif()

target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_SDL -DHAVE_SDL3)
