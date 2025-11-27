# HLFFI Examples

# Example 1: Version info
add_executable(example_version version/version.c)
target_link_libraries(example_version PRIVATE hlffi)

# TODO: Add more examples in Phase 1
# - hello_world: Load and run simple Haxe program
# - non_threaded: Engine-style tick loop
# - hot_reload: Reload code without restart
