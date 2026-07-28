# strip_debug.cmake - Strip debug symbols into a separate .debug file.
#
# Invoked at build time by add_custom_command in src/CMakeLists.txt.
# Parameters (passed via -D):
#   OBJCOPY      - Path to objcopy binary
#   TARGET_FILE  - Path to the target executable

# Check whether the binary actually contains any debug sections.
execute_process(
	COMMAND ${OBJCOPY} -h "${TARGET_FILE}"
	OUTPUT_VARIABLE _sections
)
# .debug_info is the canonical DWARF section. CodeView uses .debug$S/.debug$T
# which we intentionally do not match -- objcopy does not handle those correctly.
string(FIND "${_sections}" ".debug_info" _has_debug)

if(NOT _has_debug EQUAL -1)
	execute_process(COMMAND ${OBJCOPY} --only-keep-debug "${TARGET_FILE}" "${TARGET_FILE}.debug")
	execute_process(COMMAND ${OBJCOPY} --strip-debug --remove-section=.gnu_debuglink "${TARGET_FILE}")
	execute_process(COMMAND ${OBJCOPY} --add-gnu-debuglink="${TARGET_FILE}.debug" "${TARGET_FILE}")
endif()
