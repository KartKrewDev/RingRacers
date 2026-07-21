# Update from https://github.com/mumble-voip/ReNameNoise
# ReNameNoise unversioned
# License: BSD-3

add_library(renamenoise STATIC)
target_sources(renamenoise PRIVATE
	"src/rnn_data.c"
	"src/rnn.c"
	"src/pitch.c"
	"src/renamenoise_fft.c"
	"src/denoise.c"
	"src/renamenoise_lpc.c"
)
target_include_directories(renamenoise PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(renamenoise PRIVATE "RENAMENOISE_BUILD")
target_compile_definitions(renamenoise PRIVATE "ENABLE_ASSERTIONS")
if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
	target_compile_options(renamenoise PRIVATE -w)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(renamenoise PRIVATE /W0)
endif()
add_library(ReNameNoise::renamenoise ALIAS renamenoise)
