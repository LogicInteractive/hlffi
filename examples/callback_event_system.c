/**
 * HLFFI Callback Example: Event-Driven Game System
 *
 * Demonstrates practical C callback patterns for bidirectional C↔Haxe communication.
 * This example shows how to build an event system where Haxe game logic calls C
 * functions to handle events like player actions, UI callbacks, and game state changes.
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========== GAME STATE (C-side) ========== */

typedef struct {
    int score;
    int lives;
    char player_name[64];
    bool game_over;
} GameState;

static GameState g_state = {
    .score = 0,
    .lives = 3,
    .player_name = "Player1",
    .game_over = false
};

/* ========== C CALLBACK IMPLEMENTATIONS ========== */

/**
 * Callback: Player scored points
 * Haxe calls this when player scores
 * Signature: (Int) -> Void
 */
static hlffi_value* on_player_scored(hlffi_vm* vm, int argc, hlffi_value** args) {
    if (argc != 1) {
        fprintf(stderr, "[C] Error: on_player_scored expects 1 argument\n");
        return hlffi_value_null(vm);
    }

    int points = hlffi_value_as_int(args[0], 0);
    g_state.score += points;

    printf("[C] Player scored %d points! Total score: %d\n", points, g_state.score);

    /* Play sound effect (simulated) */
    printf("[C] *ding* Score sound played\n");

    return hlffi_value_null(vm);
}

/**
 * Callback: Player took damage
 * Haxe calls this when player is hit
 * Signature: (Int, String) -> Void
 */
static hlffi_value* on_player_damaged(hlffi_vm* vm, int argc, hlffi_value** args) {
    if (argc != 2) {
        fprintf(stderr, "[C] Error: on_player_damaged expects 2 arguments\n");
        return hlffi_value_null(vm);
    }

    int damage = hlffi_value_as_int(args[0], 0);
    const char* source = hlffi_value_as_string(args[1]);

    g_state.lives -= damage;
    printf("[C] Player hit by %s! Lost %d lives. Lives remaining: %d\n",
           source, damage, g_state.lives);

    /* Check game over */
    if (g_state.lives <= 0) {
        g_state.game_over = true;
        printf("[C] GAME OVER!\n");

        /* Notify Haxe that game ended */
        hlffi_call_static(vm, "GameCallbacks", "onGameOver", 0, NULL);
    }

    return hlffi_value_null(vm);
}

/**
 * Callback: UI button clicked
 * Haxe UI calls this when button pressed
 * Signature: (String) -> Void
 */
static hlffi_value* on_button_clicked(hlffi_vm* vm, int argc, hlffi_value** args) {
    if (argc != 1) {
        fprintf(stderr, "[C] Error: on_button_clicked expects 1 argument\n");
        return hlffi_value_null(vm);
    }

    const char* button_id = hlffi_value_as_string(args[0]);
    printf("[C] Button clicked: '%s'\n", button_id);

    /* Handle different buttons */
    if (strcmp(button_id, "restart") == 0) {
        g_state.score = 0;
        g_state.lives = 3;
        g_state.game_over = false;
        printf("[C] Game restarted!\n");

        /* Tell Haxe to restart */
        hlffi_call_static(vm, "GameCallbacks", "restartGame", 0, NULL);
    } else if (strcmp(button_id, "pause") == 0) {
        printf("[C] Game paused\n");
    } else if (strcmp(button_id, "quit") == 0) {
        printf("[C] Quit requested\n");
    }

    return hlffi_value_null(vm);
}

/**
 * Callback: Level completed
 * Haxe calls this when level finished
 * Signature: (Int, Int) -> Int  (returns bonus points)
 */
static hlffi_value* on_level_complete(hlffi_vm* vm, int argc, hlffi_value** args) {
    if (argc != 2) {
        fprintf(stderr, "[C] Error: on_level_complete expects 2 arguments\n");
        return hlffi_value_int(vm, 0);
    }

    int level = hlffi_value_as_int(args[0], 0);
    int time_seconds = hlffi_value_as_int(args[1], 0);

    printf("[C] Level %d completed in %d seconds!\n", level, time_seconds);

    /* Calculate time bonus */
    int time_bonus = 0;
    if (time_seconds < 30) {
        time_bonus = 1000;  /* Fast completion */
    } else if (time_seconds < 60) {
        time_bonus = 500;   /* Good time */
    } else {
        time_bonus = 100;   /* Slow */
    }

    int total_bonus = time_bonus + (level * 100);  /* Level bonus */
    g_state.score += total_bonus;

    printf("[C] Bonus awarded: %d points (total: %d)\n", total_bonus, g_state.score);

    return hlffi_value_int(vm, total_bonus);
}

/**
 * Callback: Get player stats
 * Haxe can query current C-side state
 * Signature: () -> String
 */
static hlffi_value* get_player_stats(hlffi_vm* vm, int argc, hlffi_value** args) {
    (void)argc; (void)args;  /* Unused */

    char stats[256];
    snprintf(stats, sizeof(stats),
             "Player: %s | Score: %d | Lives: %d | Status: %s",
             g_state.player_name,
             g_state.score,
             g_state.lives,
             g_state.game_over ? "GAME OVER" : "Playing");

    printf("[C] Stats requested: %s\n", stats);
    return hlffi_value_string(vm, stats);
}

/* ========== MAIN EXAMPLE ========== */

int main(int argc, char** argv) {
    printf("=== HLFFI Callback Example: Event-Driven Game System ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <game.hl>\n", argv[0]);
        fprintf(stderr, "\nThis example demonstrates:\n");
        fprintf(stderr, "  1. Registering C callbacks that Haxe can call\n");
        fprintf(stderr, "  2. Event-driven architecture (Haxe→C events)\n");
        fprintf(stderr, "  3. Bidirectional communication (C↔Haxe)\n");
        fprintf(stderr, "  4. Practical game event patterns\n");
        return 1;
    }

    /* Step 1: Create and initialize VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    if (hlffi_init(vm, 0, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Step 2: Load game bytecode */
    if (hlffi_load_file(vm, argv[1]) != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytecode: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    /* Step 3: Call entry point */
    if (hlffi_call_entry(vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to call entry point: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    printf("\n--- Step 1: Register C Callbacks ---\n");

    /* Register all game callbacks */
    if (!hlffi_register_callback(vm, "onPlayerScored", on_player_scored, 1)) {
        fprintf(stderr, "Failed to register onPlayerScored\n");
        goto cleanup;
    }
    printf("✓ Registered: onPlayerScored(Int)\n");

    if (!hlffi_register_callback(vm, "onPlayerDamaged", on_player_damaged, 2)) {
        fprintf(stderr, "Failed to register onPlayerDamaged\n");
        goto cleanup;
    }
    printf("✓ Registered: onPlayerDamaged(Int, String)\n");

    if (!hlffi_register_callback(vm, "onButtonClicked", on_button_clicked, 1)) {
        fprintf(stderr, "Failed to register onButtonClicked\n");
        goto cleanup;
    }
    printf("✓ Registered: onButtonClicked(String)\n");

    if (!hlffi_register_callback(vm, "onLevelComplete", on_level_complete, 2)) {
        fprintf(stderr, "Failed to register onLevelComplete\n");
        goto cleanup;
    }
    printf("✓ Registered: onLevelComplete(Int, Int) -> Int\n");

    if (!hlffi_register_callback(vm, "getPlayerStats", get_player_stats, 0)) {
        fprintf(stderr, "Failed to register getPlayerStats\n");
        goto cleanup;
    }
    printf("✓ Registered: getPlayerStats() -> String\n");

    printf("\n--- Step 2: Pass Callbacks to Haxe ---\n");

    /* Set callbacks on Haxe side */
    hlffi_value* cb;

    cb = hlffi_get_callback(vm, "onPlayerScored");
    hlffi_set_static_field(vm, "GameCallbacks", "onPlayerScored", cb);
    hlffi_value_free(cb);

    cb = hlffi_get_callback(vm, "onPlayerDamaged");
    hlffi_set_static_field(vm, "GameCallbacks", "onPlayerDamaged", cb);
    hlffi_value_free(cb);

    cb = hlffi_get_callback(vm, "onButtonClicked");
    hlffi_set_static_field(vm, "GameCallbacks", "onButtonClicked", cb);
    hlffi_value_free(cb);

    cb = hlffi_get_callback(vm, "onLevelComplete");
    hlffi_set_static_field(vm, "GameCallbacks", "onLevelComplete", cb);
    hlffi_value_free(cb);

    cb = hlffi_get_callback(vm, "getPlayerStats");
    hlffi_set_static_field(vm, "GameCallbacks", "getPlayerStats", cb);
    hlffi_value_free(cb);

    printf("✓ All callbacks set in Haxe\n");

    printf("\n--- Step 3: Run Game Simulation ---\n");

    /* Call Haxe game logic - it will trigger callbacks */
    hlffi_call_static(vm, "GameCallbacks", "simulateGame", 0, NULL);

    printf("\n--- Final State ---\n");
    printf("Score: %d\n", g_state.score);
    printf("Lives: %d\n", g_state.lives);
    printf("Game Over: %s\n", g_state.game_over ? "Yes" : "No");

    printf("\n✓ Example complete!\n");

cleanup:
    hlffi_destroy(vm);
    return 0;
}
