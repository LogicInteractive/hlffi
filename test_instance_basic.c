/**
 * test_instance_basic.c
 * Phase 4 Basic Test: Instance Members (Objects)
 *
 * Tests:
 * - hlffi_new() with no-arg constructor
 * - hlffi_get_field_int/string/bool() convenience APIs
 * - hlffi_set_field_int() convenience API
 * - hlffi_call_method_void/int/bool/string() convenience APIs
 * - GC root management with hlffi_value_free()
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_PASS(msg) printf("✓ %s\n", msg)
#define TEST_FAIL(msg) do { printf("✗ FAIL: %s\n", msg); return 1; } while(0)
#define TEST_ERROR(vm) do { printf("✗ Error: %s\n", hlffi_get_error(vm)); return 1; } while(0)

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <player.hl>\n", argv[0]);
        return 1;
    }

    printf("=== Phase 4 Basic Test: Instance Members ===\n\n");

    /* Initialize VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) TEST_FAIL("Failed to create VM");

    if (hlffi_init(vm, argc, argv) != HLFFI_OK) {
        TEST_ERROR(vm);
    }

    /* Load bytecode */
    printf("Loading %s...\n", argv[1]);
    hlffi_error_code err = hlffi_load_file(vm, argv[1]);
    if (err != HLFFI_OK) {
        TEST_ERROR(vm);
    }

    /* Call entry point (required for global_value initialization) */
    printf("Calling entry point...\n");
    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        TEST_ERROR(vm);
    }
    TEST_PASS("Entry point called successfully");

    /* ========== TEST 1: Create instance with no-arg constructor ========== */
    printf("\n--- Test 1: Create Player (no-arg constructor) ---\n");

    hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
    if (!player) {
        TEST_ERROR(vm);
    }
    TEST_PASS("Player instance created");

    /* ========== TEST 2: Get primitive field (int) ========== */
    printf("\n--- Test 2: Get primitive field (health:Int) ---\n");

    int hp = hlffi_get_field_int(player, "health", -1);
    printf("player.health = %d\n", hp);
    if (hp != 100) {
        TEST_FAIL("Expected health = 100 (from constructor)");
    }
    TEST_PASS("Got health field correctly (constructor initialized to 100)");

    /* ========== TEST 3: Get string field ========== */
    printf("\n--- Test 3: Get string field (name:String) ---\n");

    char* name_str = hlffi_get_field_string(player, "name");
    printf("player.name = \"%s\"\n", name_str ? name_str : "(null)");
    if (name_str == NULL || strcmp(name_str, "Unnamed") != 0) {
        TEST_FAIL("Expected name = \"Unnamed\" (from constructor)");
    }
    TEST_PASS("Got name field correctly (constructor initialized to \"Unnamed\")");
    if (name_str) free(name_str);

    /* ========== TEST 4: Set primitive field ========== */
    printf("\n--- Test 4: Set primitive field (health = 50) ---\n");

    if (!hlffi_set_field_int(vm, player, "health", 50)) {
        TEST_ERROR(vm);
    }

    /* Verify it was set */
    hp = hlffi_get_field_int(player, "health", -1);
    printf("player.health = %d (after set)\n", hp);
    if (hp != 50) {
        TEST_FAIL("Expected health = 50 after set");
    }
    TEST_PASS("Set health field correctly");

    /* ========== TEST 5: Call void method ========== */
    printf("\n--- Test 5: Call void method (takeDamage(25)) ---\n");

    hlffi_value* damage = hlffi_value_int(vm, 25);
    hlffi_call_method_void(player, "takeDamage", 1, &damage);
    hlffi_value_free(damage);

    /* Verify health decreased */
    hp = hlffi_get_field_int(player, "health", -1);
    printf("player.health = %d (after takeDamage(25))\n", hp);
    if (hp != 25) {
        TEST_FAIL("Expected health = 25 after takeDamage(25)");
    }
    TEST_PASS("Called takeDamage() successfully");

    /* ========== TEST 6: Call method with return value (int) ========== */
    printf("\n--- Test 6: Call method with int return (getHealth()) ---\n");

    hp = hlffi_call_method_int(player, "getHealth", 0, NULL, -1);
    printf("player.getHealth() = %d\n", hp);
    if (hp != 25) {
        TEST_FAIL("Expected getHealth() = 25");
    }
    TEST_PASS("Called getHealth() successfully");

    /* ========== TEST 7: Call method with return value (bool) ========== */
    printf("\n--- Test 7: Call method with bool return (checkAlive()) ---\n");

    bool alive = hlffi_call_method_bool(player, "checkAlive", 0, NULL, false);
    printf("player.checkAlive() = %s\n", alive ? "true" : "false");
    if (!alive) {
        TEST_FAIL("Expected checkAlive() = true (player has health > 0)");
    }
    TEST_PASS("Called checkAlive() successfully (isAlive=true from constructor)");

    /* ========== TEST 8: Call method with string return ========== */
    printf("\n--- Test 8: Call method with string return (getName()) ---\n");

    char* player_name = hlffi_call_method_string(player, "getName", 0, NULL);
    printf("player.getName() = \"%s\"\n", player_name ? player_name : "(null)");
    if (player_name == NULL || strcmp(player_name, "Unnamed") != 0) {
        TEST_FAIL("Expected getName() = \"Unnamed\" (from constructor)");
    }
    TEST_PASS("Called getName() successfully (returns \"Unnamed\" from constructor)");
    if (player_name) free(player_name);

    /* ========== TEST 9: Type checking ========== */
    printf("\n--- Test 9: Type checking (is_instance_of) ---\n");

    if (!hlffi_is_instance_of(player, "Player")) {
        TEST_FAIL("Expected player to be instance of Player");
    }
    TEST_PASS("is_instance_of works correctly");

    /* ========== TEST 10: GC root cleanup ========== */
    printf("\n--- Test 10: Free player (remove GC root) ---\n");

    hlffi_value_free(player);
    TEST_PASS("Player freed successfully");

    /* Cleanup */
    printf("\n--- Cleanup ---\n");
    hlffi_destroy(vm);
    TEST_PASS("VM destroyed");

    printf("\n=== All 10 tests passed! ===\n");
    return 0;
}
