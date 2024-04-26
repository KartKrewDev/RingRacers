target_sources(SRB2SDL2 PRIVATE
	cup-select.c
	explosions.c
	level-select.c
	gametype.c
	manual.c
	sound-test.c
	message-box.c
	pause-game.c
	pause-kick.c
	pause-replay.c
	virtual-keyboard.c
	pause-cheats.cpp
	pause-addonoptions.cpp
)

if(SRB2_CONFIG_ENABLE_DISCORDRPC)
	target_sources(SRB2SDL2 PRIVATE discord-requests.c)
endif()
