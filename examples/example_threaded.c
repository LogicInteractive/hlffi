/**
 * HLFFI Example: Threaded Mode (Mode 2)
 *
 * This example demonstrates running HashLink in THREADED mode, where the VM
 * runs on its own dedicated thread - similar to how the standard HashLink
 * runtime operates.
 *
 * In THREADED mode:
 * - The VM thread runs the Haxe entry point and processes messages
 * - The main thread can safely call into Haxe via hlffi_thread_call_sync/async
 * - This is ideal for game engines where the host has its own main loop
 *
 * USAGE:
 *   example_threaded game.hl
 *
 * COMPILE (Visual Studio):
 *   Build the example_threaded.vcxproj project
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

/* Global VM handle */
static hlffi_vm* g_vm = NULL;

/* ========== CALLBACK FUNCTIONS ========== */

/**
 * Callback to call a Haxe static method with no arguments
 */
static void call_update_callback(hlffi_vm* vm, void* userdata) {
    float* delta = (float*)userdata;

    /* Create argument */
    hlffi_value* arg = hlffi_value_float(vm, *delta);
    hlffi_value* args[] = { arg };

    /* Call Game.update(delta) */
    hlffi_value* result = hlffi_call_static(vm, "Game", "update", 1, args);

    /* Cleanup */
    if (result) hlffi_value_free(result);
    hlffi_value_free(arg);
}

/**
 * Callback to call Game.render()
 */
static void call_render_callback(hlffi_vm* vm, void* userdata) {
    (void)userdata;
    hlffi_value* result = hlffi_call_static(vm, "Game", "render", 0, NULL);
    if (result) hlffi_value_free(result);
}

/**
 * Callback to get player position
 */
typedef struct {
    float x, y;
    bool success;
} position_result_t;

static void get_position_callback(hlffi_vm* vm, void* userdata) {
    position_result_t* pos = (position_result_t*)userdata;
    pos->success = false;

    /* Call Player.getX() */
    hlffi_value* x_result = hlffi_call_static(vm, "Player", "getX", 0, NULL);
    if (x_result) {
        pos->x = hlffi_value_as_float(x_result, 0.0f);
        hlffi_value_free(x_result);

        /* Call Player.getY() */
        hlffi_value* y_result = hlffi_call_static(vm, "Player", "getY", 0, NULL);
        if (y_result) {
            pos->y = hlffi_value_as_float(y_result, 0.0f);
            hlffi_value_free(y_result);
            pos->success = true;
        }
    }
}

/**
 * Callback to set player position
 */
typedef struct {
    float x, y;
} set_position_args_t;

static void set_position_callback(hlffi_vm* vm, void* userdata) {
    set_position_args_t* args_data = (set_position_args_t*)userdata;

    hlffi_value* x_arg = hlffi_value_float(vm, args_data->x);
    hlffi_value* y_arg = hlffi_value_float(vm, args_data->y);
    hlffi_value* args[] = { x_arg, y_arg };

    hlffi_value* result = hlffi_call_static(vm, "Player", "setPosition", 2, args);

    if (result) hlffi_value_free(result);
    hlffi_value_free(x_arg);
    hlffi_value_free(y_arg);
}

/* ========== ASYNC CALLBACK ========== */

static volatile int async_save_complete = 0;

static void save_complete_callback(hlffi_vm* vm, void* result, void* userdata) {
    (void)vm;
    (void)result;
    (void)userdata;
    async_save_complete = 1;
    printf("  [Async] Save completed!\n");
}

static void save_game_callback(hlffi_vm* vm, void* userdata) {
    (void)userdata;
    printf("  [VM Thread] Saving game...\n");
    hlffi_value* result = hlffi_call_static(vm, "Game", "save", 0, NULL);
    if (result) hlffi_value_free(result);
}

/* ========== MAIN ========== */

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <game.hl>\n", argv[0]);
        fprintf(stderr, "\nThis example expects a Haxe bytecode file with:\n");
        fprintf(stderr, "  - Game.update(delta:Float)\n");
        fprintf(stderr, "  - Game.render()\n");
        fprintf(stderr, "  - Game.save()\n");
        fprintf(stderr, "  - Player.getX(), Player.getY()\n");
        fprintf(stderr, "  - Player.setPosition(x:Float, y:Float)\n");
        return 1;
    }

    const char* hl_file = argv[1];

    printf("==============================================\n");
    printf("HLFFI Threaded Mode Example\n");
    printf("==============================================\n");
    printf("Bytecode: %s\n\n", hl_file);

    /* Step 1: Create VM */
    printf("[1] Creating VM...\n");
    g_vm = hlffi_create();
    if (!g_vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }

    /* Step 2: Set THREADED mode BEFORE init */
    printf("[2] Setting THREADED mode...\n");
    hlffi_set_integration_mode(g_vm, HLFFI_MODE_THREADED);

    /* Step 3: Initialize */
    printf("[3] Initializing VM...\n");
    if (hlffi_init(g_vm, argc, argv) != HLFFI_OK) {
        fprintf(stderr, "Failed to initialize VM: %s\n", hlffi_get_error(g_vm));
        hlffi_destroy(g_vm);
        return 1;
    }

    /* Step 4: Load bytecode */
    printf("[4] Loading bytecode...\n");
    if (hlffi_load_file(g_vm, hl_file) != HLFFI_OK) {
        fprintf(stderr, "Failed to load bytecode: %s\n", hlffi_get_error(g_vm));
        hlffi_destroy(g_vm);
        return 1;
    }

    /* Step 5: Start VM thread
     * This spawns the dedicated VM thread which:
     * - Calls the Haxe entry point (main())
     * - Enters message processing loop
     * - Handles sync/async calls from other threads
     */
    printf("[5] Starting VM thread...\n");
    if (hlffi_thread_start(g_vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to start thread: %s\n", hlffi_get_error(g_vm));
        hlffi_destroy(g_vm);
        return 1;
    }

    printf("[6] VM thread started. Running main loop...\n\n");

    /* Give the VM thread time to initialize */
    sleep_ms(100);

    /* ========== MAIN LOOP ========== */
    /* This simulates a game engine's main loop running on the main thread */

    printf("--- Simulated Game Loop (5 frames) ---\n\n");

    for (int frame = 0; frame < 5; frame++) {
        printf("Frame %d:\n", frame);

        /* Calculate delta time (simulated) */
        float delta = 0.016667f;  /* ~60 FPS */

        /* Call Game.update(delta) synchronously */
        printf("  Calling Game.update(%.4f)...\n", delta);
        if (hlffi_thread_call_sync(g_vm, call_update_callback, &delta) != HLFFI_OK) {
            fprintf(stderr, "  Failed to call update: %s\n", hlffi_get_error(g_vm));
        }

        /* Call Game.render() synchronously */
        printf("  Calling Game.render()...\n");
        if (hlffi_thread_call_sync(g_vm, call_render_callback, NULL) != HLFFI_OK) {
            fprintf(stderr, "  Failed to call render: %s\n", hlffi_get_error(g_vm));
        }

        /* Get player position */
        position_result_t pos = {0};
        if (hlffi_thread_call_sync(g_vm, get_position_callback, &pos) == HLFFI_OK && pos.success) {
            printf("  Player position: (%.2f, %.2f)\n", pos.x, pos.y);
        }

        /* Every other frame, move the player */
        if (frame % 2 == 0) {
            set_position_args_t new_pos = { .x = frame * 10.0f, .y = frame * 5.0f };
            printf("  Setting player position to (%.2f, %.2f)...\n", new_pos.x, new_pos.y);
            hlffi_thread_call_sync(g_vm, set_position_callback, &new_pos);
        }

        printf("\n");

        /* Simulate frame time */
        sleep_ms(100);
    }

    /* ========== ASYNC OPERATION ========== */
    printf("--- Async Save Operation ---\n\n");

    /* Fire-and-forget save with callback notification */
    printf("Triggering async save...\n");
    async_save_complete = 0;

    if (hlffi_thread_call_async(g_vm, save_game_callback, save_complete_callback, NULL) != HLFFI_OK) {
        fprintf(stderr, "Failed to start async save: %s\n", hlffi_get_error(g_vm));
    }

    /* Continue doing other work while save happens */
    printf("Main thread continues working...\n");
    for (int i = 0; i < 10 && !async_save_complete; i++) {
        printf("  Main thread tick %d...\n", i);
        sleep_ms(50);
    }

    if (async_save_complete) {
        printf("Save completed successfully!\n");
    } else {
        printf("Save timed out.\n");
    }

    printf("\n");

    /* ========== CLEANUP ========== */
    printf("--- Cleanup ---\n\n");

    /* Stop the VM thread gracefully */
    printf("Stopping VM thread...\n");
    if (hlffi_thread_stop(g_vm) != HLFFI_OK) {
        fprintf(stderr, "Failed to stop thread: %s\n", hlffi_get_error(g_vm));
    }

    /* Destroy VM */
    printf("Destroying VM...\n");
    hlffi_destroy(g_vm);
    g_vm = NULL;

    printf("\nDone!\n");
    return 0;
}
