# Update from https://github.com/nlohmann/json/releases
add_library(nlohmann_json INTERFACE "include/nlohmann/json.hpp" "include/nlohmann/json_fwd.hpp")
target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
