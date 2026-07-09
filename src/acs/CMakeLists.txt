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

# This breaks Apple Clang 14 compile. It should be totally
# unecessary since even though vm/CMakeLists.txt sets
# CMAKE_CXX_FLAGS, it is in a lower scope.
#set(ACSVM_NOFLAGS ON)

set(ACSVM_SHARED OFF)
set(ACSVM_INSTALL_API OFF)
set(ACSVM_INSTALL_DOC OFF)
set(ACSVM_INSTALL_EXE OFF)
set(ACSVM_INSTALL_LIB OFF)
add_subdirectory(vm)

target_link_libraries(SRB2SDL2 PRIVATE acsvm)
