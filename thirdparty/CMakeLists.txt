add_subdirectory(glm)
if(SRB2_CONFIG_ENABLE_DISCORDRPC)
	add_subdirectory(discord-rpc)
endif()
add_subdirectory(xmp-lite)
add_subdirectory(imgui)

add_subdirectory(nlohmann-json)

add_subdirectory(tcbrindle_span)
add_subdirectory(stb)
add_subdirectory(glad)
add_subdirectory(tracy)

add_subdirectory(libwebm)

add_subdirectory(fmt)

add_subdirectory(vulkan-headers)
add_subdirectory(volk)
add_subdirectory(vma)

add_subdirectory(renamenoise)
