/**
 * Phase 3 Test: Static Fields and Methods
 * Tests value boxing/unboxing, static field access, and static method calls
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 3 Test: Static Members & Values ===\n\n");

    // Create and initialize VM
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }

    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Init failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    err = hlffi_load_file(vm, argv[1]);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Load failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // CRITICAL: Call entry point to initialize static fields
    printf("Calling entry point to initialize static fields...\n");
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Entry point failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("✓ Entry point called, static fields initialized\n\n");

    // TEST 1: Get static field (int)
    printf("--- Test 1: Get Static Int Field ---\n");
    hlffi_value* score = hlffi_get_static_field(vm, "Game", "score");
    if (score) {
        int score_val = hlffi_value_as_int(score, -1);
        printf("Game.score = %d\n", score_val);
        free(score);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    printf("\n");

    // TEST 2: Get static field (string)
    printf("--- Test 2: Get Static String Field ---\n");
    hlffi_value* name = hlffi_get_static_field(vm, "Game", "playerName");
    if (name) {
        char* name_str = hlffi_value_as_string(name);
        printf("Game.playerName = \"%s\"\n", name_str ? name_str : "(null)");
        if (name_str) free(name_str);
        free(name);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    printf("\n");

    // TEST 3: Set static field (int)
    printf("--- Test 3: Set Static Int Field ---\n");
    hlffi_value* new_score = hlffi_value_int(vm, 999);
    err = hlffi_set_static_field(vm, "Game", "score", new_score);
    if (err == HLFFI_OK) {
        printf("✓ Set Game.score = 999\n");

        // Verify it was set
        hlffi_value* verify = hlffi_get_static_field(vm, "Game", "score");
        if (verify) {
            printf("  Verified: Game.score = %d\n", hlffi_value_as_int(verify, -1));
            free(verify);
        }
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    free(new_score);
    printf("\n");

    // TEST 4: Set static field (string)
    printf("--- Test 4: Set Static String Field ---\n");
    hlffi_value* new_name = hlffi_value_string(vm, "Hero");
    err = hlffi_set_static_field(vm, "Game", "playerName", new_name);
    if (err == HLFFI_OK) {
        printf("✓ Set Game.playerName = \"Hero\"\n");

        // Verify
        hlffi_value* verify = hlffi_get_static_field(vm, "Game", "playerName");
        if (verify) {
            char* verify_str = hlffi_value_as_string(verify);
            printf("  Verified: Game.playerName = \"%s\"\n", verify_str ? verify_str : "(null)");
            if (verify_str) free(verify_str);
            free(verify);
        }
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    free(new_name);
    printf("\n");

    // TEST 5: Call static method with no args
    printf("--- Test 5: Call Static Method (no args) ---\n");
    hlffi_value* result = hlffi_call_static(vm, "Game", "start", 0, NULL);
    if (result) {
        printf("✓ Called Game.start()\n");
        free(result);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    printf("\n");

    // TEST 6: Call static method with int arg
    printf("--- Test 6: Call Static Method (int arg) ---\n");
    hlffi_value* points = hlffi_value_int(vm, 250);
    hlffi_value* args1[1] = {points};
    result = hlffi_call_static(vm, "Game", "addPoints", 1, args1);
    if (result) {
        printf("✓ Called Game.addPoints(250)\n");
        free(result);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    free(points);
    printf("\n");

    // TEST 7: Call static method with return value (int)
    printf("--- Test 7: Call Static Method (returns int) ---\n");
    result = hlffi_call_static(vm, "Game", "getScore", 0, NULL);
    if (result) {
        int current_score = hlffi_value_as_int(result, -1);
        printf("✓ Game.getScore() returned: %d\n", current_score);
        free(result);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    printf("\n");

    // TEST 8: Call static method with string arg and return
    printf("--- Test 8: Call Static Method (string -> string) ---\n");
    hlffi_value* greeting_name = hlffi_value_string(vm, "C");
    hlffi_value* args2[1] = {greeting_name};
    result = hlffi_call_static(vm, "Game", "greet", 1, args2);
    if (result) {
        char* greeting = hlffi_value_as_string(result);
        printf("✓ Game.greet(\"C\") returned: \"%s\"\n", greeting ? greeting : "(null)");
        if (greeting) free(greeting);
        free(result);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    free(greeting_name);
    printf("\n");

    // TEST 9: Call static method with multiple args
    printf("--- Test 9: Call Static Method (multiple args) ---\n");
    hlffi_value* a = hlffi_value_int(vm, 42);
    hlffi_value* b = hlffi_value_int(vm, 13);
    hlffi_value* args3[2] = {a, b};
    result = hlffi_call_static(vm, "Game", "add", 2, args3);
    if (result) {
        int sum = hlffi_value_as_int(result, -1);
        printf("✓ Game.add(42, 13) returned: %d\n", sum);
        free(result);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    free(a);
    free(b);
    printf("\n");

    // TEST 10: Call static method with float args
    printf("--- Test 10: Call Static Method (float args) ---\n");
    hlffi_value* x = hlffi_value_float(vm, 2.5);
    hlffi_value* y = hlffi_value_float(vm, 4.0);
    hlffi_value* args4[2] = {x, y};
    result = hlffi_call_static(vm, "Game", "multiply", 2, args4);
    if (result) {
        double product = hlffi_value_as_float(result, -1.0);
        printf("✓ Game.multiply(2.5, 4.0) returned: %.1f\n", product);
        free(result);
    } else {
        printf("✗ Failed: %s\n", hlffi_get_error(vm));
    }
    free(x);
    free(y);
    printf("\n");

    // Cleanup
    printf("--- Cleanup ---\n");
    hlffi_destroy(vm);
    printf("✓ VM destroyed\n\n");

    printf("=== All Phase 3 Tests Complete ===\n");
    return 0;
}
