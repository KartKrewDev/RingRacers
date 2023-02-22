CPMAddPackage(
	NAME DiscordRPC
	VERSION 3.4.0
	URL "https://github.com/discord/discord-rpc/archive/refs/tags/v3.4.0.zip"
	EXCLUDE_FROM_ALL ON
	DOWNLOAD_ONLY ON
)

if(DiscordRPC_ADDED)
	set(DiscordRPC_SOURCES
		include/discord_rpc.h
		include/discord_register.h

		src/discord_rpc.cpp
		src/rpc_connection.h
		src/rpc_connection.cpp
		src/serialization.h
		src/serialization.cpp
		src/connection.h
		src/backoff.h
		src/msg_queue.h
	)
	list(TRANSFORM DiscordRPC_SOURCES PREPEND "${DiscordRPC_SOURCE_DIR}/")

	# Discord RPC is always statically linked because it's tiny.
	add_library(discord-rpc STATIC ${DiscordRPC_SOURCES})
	add_library(DiscordRPC::DiscordRPC ALIAS discord-rpc)

	target_include_directories(discord-rpc PUBLIC "${DiscordRPC_SOURCE_DIR}/include")
	target_compile_features(discord-rpc PUBLIC cxx_std_11)
	target_link_libraries(discord-rpc PRIVATE RapidJSON::RapidJSON)

	# Platform-specific connection and register impls
	if(WIN32)
		target_compile_definitions(discord-rpc PUBLIC -DDISCORD_WINDOWS)
		target_sources(discord-rpc PRIVATE
			"${DiscordRPC_SOURCE_DIR}/src/connection_win.cpp"
			"${DiscordRPC_SOURCE_DIR}/src/discord_register_win.cpp"
		)
		target_link_libraries(discord-rpc PRIVATE psapi advapi32)
	endif()

	if(UNIX)
		target_sources(discord-rpc PRIVATE
			"${DiscordRPC_SOURCE_DIR}/src/connection_unix.cpp"
		)

		if(APPLE)
			target_compile_definitions(discord-rpc PUBLIC -DDISCORD_OSX)
			target_sources(discord-rpc PRIVATE
				"${DiscordRPC_SOURCE_DIR}/src/discord_register_osx.m"
			)
			target_link_libraries(discord-rpc PUBLIC "-framework AppKit")
		endif()

		if(UNIX AND NOT APPLE)
			target_compile_definitions(discord-rpc PUBLIC -DDISCORD_LINUX)
			target_sources(discord-rpc PRIVATE
				"${DiscordRPC_SOURCE_DIR}/src/discord_register_linux.cpp"
			)
			target_link_libraries(discord-rpc PUBLIC pthread)
		endif()
	endif()
endif()
