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
	dosstr.c
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

	#target_link_libraries(SRB2SDL2 PRIVATE SRB2Core)
	set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")

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

#### Installation ####
if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	install(TARGETS SRB2SDL2
		BUNDLE DESTINATION .
	)
	set_property(TARGET SRB2SDL2 PROPERTY INSTALL_RPATH_USE_LINK_PATH ON)
else()
	install(TARGETS SRB2SDL2 SRB2SDL2
		RUNTIME DESTINATION .
	)
	if ((${CMAKE_BUILD_TYPE} MATCHES Debug) OR (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo))
		set(SRB2_DEBUG_INSTALL OFF CACHE BOOL "Insert *.debug file into the install directory or package.")
		if (${SRB2_DEBUG_INSTALL})
			install(FILES $<TARGET_FILE:SRB2SDL2>.debug
				DESTINATION .
				OPTIONAL
			)
		endif()
	endif()
endif()

# Mac bundle fixup
# HACK: THIS IS IMPORTANT! See the escaped \${CMAKE_INSTALL_PREFIX}? This
# makes it so that var is evaluated LATER during cpack, not right now!
# This fixes the quirk where the bundled libraries don't land in the final package
# https://cmake.org/pipermail/cmake/2011-March/043532.html
#
# HOWEVER: ${CPACK_PACKAGE_DESCRIPTION_SUMMARY} is NOT escaped, because that var
# is only available to us at this step. Read the link: ${CMAKE_INSTALL_PREFIX} at
# this current step points to the CMAKE build folder, NOT the folder that CPACK uses.
# Therefore, it makes sense to escape that var, but not the other.
if("${CMAKE_SYSTEM_NAME}" MATCHES Darwin)
	install(CODE "
		include(BundleUtilities)
		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${CPACK_PACKAGE_DESCRIPTION_SUMMARY}.app\"
			\"\"
			/Library/Frameworks
		)"
	)
endif()
