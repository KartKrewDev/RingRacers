macro(export)
endmacro()

if(SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES)
	set(SRB2_INTERNAL_LIBRARY_TYPE SHARED)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES OFF)
else()
	set(SRB2_INTERNAL_LIBRARY_TYPE STATIC)
	set(NOT_SRB2_CONFIG_SHARED_INTERNAL_LIBRARIES ON)
endif()

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
include("cpm-sdl2.cmake")
	include("cpm-zlib.cmake")
	include("cpm-png.cmake")
	include("cpm-curl.cmake")
	include("cpm-libgme.cmake")
endif()

include("cpm-rapidjson.cmake")
include("cpm-discordrpc.cmake")
include("cpm-xmp-lite.cmake")
include("cpm-fmt.cmake")
include("cpm-imgui.cmake")

if(NOT "${SRB2_CONFIG_SYSTEM_LIBRARIES}")
	CPMAddPackage(
		NAME acsvm
		VERSION 0
		URL "https://github.com/DavidPH/ACSVM/archive/7011af443dd03e8592d7810b0b91f46c49bdde59.zip"
		EXCLUDE_FROM_ALL ON
		DOWNLOAD_ONLY YES
	)

	if(acsvm_ADDED)
		# Sal -- While ACSVM can be built as a shared library, a lot of its options are
		# tied to directories existing, because the project suggests just copying it into
		# your own project directly. I don't want us to do that, so I made our own target.
		set(
			acsvm_SOURCES

			ACSVM/Action.cpp
			ACSVM/Action.hpp
			ACSVM/Array.cpp
			ACSVM/Array.hpp
			ACSVM/BinaryIO.cpp
			ACSVM/BinaryIO.hpp
			ACSVM/CallFunc.cpp
			ACSVM/CallFunc.hpp
			ACSVM/Code.hpp
			ACSVM/CodeData.cpp
			ACSVM/CodeData.hpp
			ACSVM/CodeList.hpp
			ACSVM/Environment.cpp
			ACSVM/Environment.hpp
			ACSVM/Error.cpp
			ACSVM/Error.hpp
			ACSVM/Function.cpp
			ACSVM/Function.hpp
			ACSVM/HashMap.hpp
			ACSVM/HashMapFixed.hpp
			ACSVM/ID.hpp
			ACSVM/Init.cpp
			ACSVM/Init.hpp
			ACSVM/Jump.cpp
			ACSVM/Jump.hpp
			ACSVM/Module.cpp
			ACSVM/Module.hpp
			ACSVM/ModuleACS0.cpp
			ACSVM/ModuleACSE.cpp
			ACSVM/PrintBuf.cpp
			ACSVM/PrintBuf.hpp
			ACSVM/Scope.cpp
			ACSVM/Scope.hpp
			ACSVM/Script.cpp
			ACSVM/Script.hpp
			ACSVM/Serial.cpp
			ACSVM/Serial.hpp
			ACSVM/Stack.hpp
			ACSVM/Store.hpp
			ACSVM/String.cpp
			ACSVM/String.hpp
			ACSVM/Thread.cpp
			ACSVM/Thread.hpp
			ACSVM/ThreadExec.cpp
			ACSVM/Tracer.cpp
			ACSVM/Tracer.hpp
			ACSVM/Types.hpp
			ACSVM/Vector.hpp

			Util/Floats.cpp
			Util/Floats.hpp
		)
		list(TRANSFORM acsvm_SOURCES PREPEND "${acsvm_SOURCE_DIR}/")
		add_library(acsvm "${SRB2_INTERNAL_LIBRARY_TYPE}" ${acsvm_SOURCES})

		target_compile_features(acsvm PRIVATE cxx_std_11)

		target_include_directories(acsvm INTERFACE "${acsvm_SOURCE_DIR}")

		target_link_libraries(acsvm PRIVATE acsvm::acsvm)
		add_library(acsvm::acsvm ALIAS acsvm)
	endif()
endif()

add_subdirectory(tcbrindle_span)
add_subdirectory(stb_vorbis)
add_subdirectory(glad)
