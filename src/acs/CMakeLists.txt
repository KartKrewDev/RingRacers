target_sources(SRB2SDL2 PRIVATE
	environment.cpp
	environment.hpp
	thread.cpp
	thread.hpp
	call-funcs.cpp
	call-funcs.hpp
	stream.cpp
	stream.hpp
	interface.cpp
	interface.h
)

target_include_directories(SRB2SDL2 PRIVATE vm) # This sucks

set(ACSVM_NOFLAGS ON)
set(ACSVM_SHARED OFF)
add_subdirectory(vm)

target_link_libraries(SRB2SDL2 PRIVATE acsvm)
