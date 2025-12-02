# HLFFI Tests

# ========== HLC Mode Tests ==========
if(HLFFI_HLC_MODE)
    set(HLC_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../test/hlc_test")

    # --- Test 1: Basic HLC test (Main.hx) ---
    set(HLC_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../test/hlc_output")
    if(EXISTS "${HLC_OUTPUT_DIR}/main.c")
        file(GLOB_RECURSE HLC_ALL_SOURCES "${HLC_OUTPUT_DIR}/*.c")
        list(FILTER HLC_ALL_SOURCES EXCLUDE REGEX "main\\.c$")

        add_executable(test_hlc
            "${HLC_TEST_DIR}/test_hlc.c"
            "${HLC_TEST_DIR}/hlc_entry.c"
            ${HLC_ALL_SOURCES}
        )

        target_include_directories(test_hlc PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/hashlink/src
            ${HASHLINK_INCLUDE_DIR}
            ${HLC_OUTPUT_DIR}
        )

        target_link_libraries(test_hlc PRIVATE hlffi)
        target_compile_definitions(test_hlc PRIVATE HLFFI_HLC_MODE=1 HL_MAKE=1 _CONSOLE)

        if(MSVC)
            target_compile_options(test_hlc PRIVATE /W3)
        else()
            target_compile_options(test_hlc PRIVATE -w)
        endif()

        add_test(NAME test_hlc COMMAND test_hlc)
        message(STATUS "HLC basic test configured")
    else()
        message(STATUS "HLC output not found at ${HLC_OUTPUT_DIR}")
        message(STATUS "Run: cd test && haxe -hl hlc_output/main.c -main Main")
    endif()

    # --- Test 2: HLC Test Suite (comprehensive tests) ---
    set(HLC_SUITE_DIR "${HLC_TEST_DIR}/hlc_suite_output")
    if(EXISTS "${HLC_SUITE_DIR}/main.c")
        file(GLOB_RECURSE HLC_SUITE_SOURCES "${HLC_SUITE_DIR}/*.c")
        list(FILTER HLC_SUITE_SOURCES EXCLUDE REGEX "main\\.c$")

        add_executable(test_hlc_suite
            "${HLC_TEST_DIR}/test_hlc_suite.c"
            "${HLC_TEST_DIR}/hlc_suite_entry.c"
            ${HLC_SUITE_SOURCES}
        )

        target_include_directories(test_hlc_suite PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/hashlink/src
            ${HASHLINK_INCLUDE_DIR}
            ${HLC_SUITE_DIR}
        )

        target_link_libraries(test_hlc_suite PRIVATE hlffi)
        target_compile_definitions(test_hlc_suite PRIVATE HLFFI_HLC_MODE=1 HL_MAKE=1 _CONSOLE)

        if(MSVC)
            target_compile_options(test_hlc_suite PRIVATE /W3)
        else()
            target_compile_options(test_hlc_suite PRIVATE -w)
        endif()

        add_test(NAME test_hlc_suite COMMAND test_hlc_suite)
        message(STATUS "HLC test suite configured")
    else()
        message(STATUS "HLC test suite not found at ${HLC_SUITE_DIR}")
        message(STATUS "Run: cd test/hlc_test && haxe -hl hlc_suite_output/main.c -main HlcTestSuite")
    endif()
endif()

# ========== JIT Mode Tests ==========
if(NOT HLFFI_HLC_MODE)
    # JIT Test Suite - runs multiple .hl bytecode files
    set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../test")

    add_executable(test_jit_suite
        "${TEST_DIR}/jit_test/test_jit_suite.c"
    )
    target_link_libraries(test_jit_suite PRIVATE hlffi)

    # Pass test directory to executable
    add_test(
        NAME test_jit_suite
        COMMAND test_jit_suite "${TEST_DIR}/"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    message(STATUS "JIT test suite configured")
endif()
