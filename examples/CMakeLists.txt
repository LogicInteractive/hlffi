# HLFFI Examples

# Example 1: Version info (works in both JIT and HLC mode)
add_executable(example_version version/version.c)
target_link_libraries(example_version PRIVATE hlffi_jit libhl)

# Example 2: Hello world - requires JIT mode (loads .hl bytecode)
add_executable(example_hello_world hello_world/hello_world.c)
target_link_libraries(example_hello_world PRIVATE hlffi_jit libhl)

# TODO: Add more examples
# - non_threaded: Engine-style tick loop
# - hot_reload: Reload code without restart
