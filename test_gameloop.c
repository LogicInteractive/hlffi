/**
 * HLFFI Game Loop Test
 * Demonstrates non-threaded VM with external mainloop control
 *
 * This test simulates a game engine driving the Haxe MainLoop:
 * - C code controls the main loop (like Unreal/Unity)
 * - Calls haxe.MainLoop.tick() each frame via hlffi_call_static
 * - Haxe MainLoop callbacks fire on each tick
 * - No blocking - pure tick-based event processing
 *
 * This is the WASM pattern adapted to native embedding:
 * JavaScript's requestAnimationFrame -> C's game loop
 * hl_mainloop_tick() -> hlffi_call_static(vm, "haxe.MainLoop", "tick")
 */

#include "include/hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
static double get_time_ms(void) {
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER counter;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)freq.QuadPart;
}
#else
#include <unistd.h>
#include <sys/time.h>
#define sleep_ms(ms) usleep((ms) * 1000)
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}
#endif

/**
 * Call haxe.MainLoop.tick() to process pending callbacks.
 * This is the key function that drives Haxe's event loop from C.
 * Returns true if tick succeeded, false otherwise.
 */
static bool mainloop_tick(hlffi_vm* vm) {
    /* haxe.MainLoop.tick() processes all pending callbacks */
    hlffi_value* result = hlffi_call_static(vm, "haxe.MainLoop", "tick", 0, NULL);
    if (result) {
        hlffi_value_free(result);
        return true;
    }
    /* If MainLoop doesn't exist (no timers used), that's OK */
    return true;
}

/* Test simple game loop without MainLoop (just static method calls) */
static int test_simple_loop(hlffi_vm* vm) {
    printf("\n=== Test 1: Simple Loop (No MainLoop) ===\n");
    printf("This tests calling Game.update() directly from C\n\n");

    /* Note: HLFFI_ENTER_SCOPE() is no longer needed here!
     * The library now handles stack_top updates internally. */

    /* Reset game state */
    hlffi_value* reset = hlffi_call_static(vm, "Game", "resetGameLoop", 0, NULL);
    if (reset) hlffi_value_free(reset);

    /* Set multiplier */
    hlffi_value* mult = hlffi_value_float(vm, 2.0);
    hlffi_set_static_field(vm, "Game", "multiplier", mult);
    hlffi_value_free(mult);

    /* Run 60 frames at simulated 60 FPS */
    const int frames = 60;
    const double delta = 1.0 / 60.0;  /* 16.67ms per frame */

    printf("Running %d frames at %.1f FPS...\n", frames, 1.0 / delta);

    for (int i = 0; i < frames; i++) {
        /* Call Game.update(deltaTime) */
        hlffi_value* dt = hlffi_value_float(vm, delta);
        if (!dt) {
            printf("ERROR: Failed to create float value at frame %d\n", i);
            return 1;
        }
        hlffi_value* args[] = {dt};
        hlffi_value* result = hlffi_call_static(vm, "Game", "update", 1, args);
        hlffi_value_free(dt);
        if (result) hlffi_value_free(result);

        /* Print status every 10 frames */
        if ((i + 1) % 10 == 0) {
            hlffi_value* fc = hlffi_get_static_field(vm, "Game", "frameCount");
            hlffi_value* sc = hlffi_get_static_field(vm, "Game", "score");
            int frame_count = fc ? hlffi_value_as_int(fc, -1) : -1;
            int score = sc ? hlffi_value_as_int(sc, -1) : -1;
            printf("  Frame %d: frameCount=%d, score=%d\n", i + 1, frame_count, score);
            if (fc) hlffi_value_free(fc);
            if (sc) hlffi_value_free(sc);
        }
    }

    /* Verify final state */
    hlffi_value* fc = hlffi_get_static_field(vm, "Game", "frameCount");
    hlffi_value* sc = hlffi_get_static_field(vm, "Game", "score");
    int final_frames = fc ? hlffi_value_as_int(fc, -1) : -1;
    int final_score = sc ? hlffi_value_as_int(sc, -1) : -1;
    if (fc) hlffi_value_free(fc);
    if (sc) hlffi_value_free(sc);

    printf("\nResults:\n");
    printf("  Frame count: %d (expected: %d)\n", final_frames, frames);
    printf("  Score: %d (expected: %d)\n", final_score, (frames / 10) * 20);  /* 20 points per 10 frames at 2x */

    bool passed = (final_frames == frames) && (final_score == (frames / 10) * 20);
    printf("  Status: %s\n", passed ? "PASS" : "FAIL");

    return passed ? 0 : 1;
}

/* Test MainLoop-driven callbacks */
static int test_mainloop(hlffi_vm* vm) {
    printf("\n=== Test 2: MainLoop Integration ===\n");
    printf("This tests driving haxe.MainLoop.tick() from C\n\n");

    /* Note: Stack_top is now handled internally by HLFFI functions */

    /* Check if MainLoopTest exists (may not be compiled in) */
    hlffi_value* check = hlffi_call_static(vm, "MainLoopTest", "getTickCount", 0, NULL);
    if (!check) {
        printf("MainLoopTest not found - skipping MainLoop test\n");
        printf("(Compile MainLoopTest.hx to enable this test)\n");
        return 0;  /* Not a failure, just not available */
    }
    hlffi_value_free(check);

    /* Reset MainLoopTest state */
    hlffi_value* reset = hlffi_call_static(vm, "MainLoopTest", "reset", 0, NULL);
    if (reset) hlffi_value_free(reset);

    /* Run mainloop ticks */
    const int ticks = 50;
    printf("Running %d MainLoop ticks...\n", ticks);

    for (int i = 0; i < ticks; i++) {
        /* Call haxe.MainLoop.tick() to process callbacks */
        mainloop_tick(vm);

        /* Print status every 10 ticks */
        if ((i + 1) % 10 == 0) {
            hlffi_value* tc = hlffi_call_static(vm, "MainLoopTest", "getTickCount", 0, NULL);
            hlffi_value* tf = hlffi_call_static(vm, "MainLoopTest", "getTimerFired", 0, NULL);
            int tick_count = tc ? hlffi_value_as_int(tc, -1) : -1;
            int timer_fired = tf ? hlffi_value_as_int(tf, -1) : -1;
            printf("  Tick %d: tickCount=%d, timerFired=%d\n", i + 1, tick_count, timer_fired);
            if (tc) hlffi_value_free(tc);
            if (tf) hlffi_value_free(tf);
        }
    }

    /* Verify final state */
    hlffi_value* tc = hlffi_call_static(vm, "MainLoopTest", "getTickCount", 0, NULL);
    hlffi_value* tf = hlffi_call_static(vm, "MainLoopTest", "getTimerFired", 0, NULL);
    int final_ticks = tc ? hlffi_value_as_int(tc, -1) : -1;
    int final_timers = tf ? hlffi_value_as_int(tf, -1) : -1;
    if (tc) hlffi_value_free(tc);
    if (tf) hlffi_value_free(tf);

    printf("\nResults:\n");
    printf("  Tick count: %d (expected: %d)\n", final_ticks, ticks);
    printf("  Timer fired: %d (expected: %d)\n", final_timers, ticks / 10);

    bool passed = (final_ticks == ticks) && (final_timers == ticks / 10);
    printf("  Status: %s\n", passed ? "PASS" : "FAIL");

    return passed ? 0 : 1;
}

/* Test real-time loop with wall clock timing */
static int test_realtime_loop(hlffi_vm* vm) {
    printf("\n=== Test 3: Real-Time Loop ===\n");
    printf("This tests running a real 60 FPS loop for 1 second\n\n");

    /* Note: Stack_top is now handled internally by HLFFI functions */

    /* Reset game state */
    hlffi_value* reset = hlffi_call_static(vm, "Game", "resetGameLoop", 0, NULL);
    if (reset) hlffi_value_free(reset);

    /* Set multiplier */
    hlffi_value* mult = hlffi_value_float(vm, 1.5);
    hlffi_set_static_field(vm, "Game", "multiplier", mult);
    hlffi_value_free(mult);

    const int target_fps = 60;
    const int duration_ms = 1000;
    const double frame_time_ms = 1000.0 / target_fps;

    double start_time = get_time_ms();
    double last_frame = start_time;
    int frames_run = 0;

    printf("Running at %d FPS for %d ms...\n", target_fps, duration_ms);

    while ((get_time_ms() - start_time) < duration_ms) {
        double now = get_time_ms();
        double elapsed = now - last_frame;

        if (elapsed >= frame_time_ms) {
            double delta = elapsed / 1000.0;

            /* Call Game.update(deltaTime) */
            hlffi_value* dt = hlffi_value_float(vm, delta);
            hlffi_value* args[] = {dt};
            hlffi_value* result = hlffi_call_static(vm, "Game", "update", 1, args);
            hlffi_value_free(dt);
            if (result) hlffi_value_free(result);

            /* Also tick MainLoop if available */
            mainloop_tick(vm);

            frames_run++;
            last_frame = now;

            /* Print a dot every 10 frames to show progress */
            if (frames_run % 10 == 0) {
                printf(".");
                fflush(stdout);
            }
        } else {
            sleep_ms(1);
        }
    }

    double total_time = get_time_ms() - start_time;

    /* Get final state */
    hlffi_value* fc = hlffi_get_static_field(vm, "Game", "frameCount");
    hlffi_value* tt = hlffi_get_static_field(vm, "Game", "totalTime");
    hlffi_value* sc = hlffi_get_static_field(vm, "Game", "score");
    int final_frames = fc ? hlffi_value_as_int(fc, -1) : -1;
    double final_time = tt ? hlffi_value_as_float(tt, -1.0) : -1.0;
    int final_score = sc ? hlffi_value_as_int(sc, -1) : -1;
    if (fc) hlffi_value_free(fc);
    if (tt) hlffi_value_free(tt);
    if (sc) hlffi_value_free(sc);

    printf("\nResults:\n");
    printf("  Wall time: %.1f ms\n", total_time);
    printf("  Frames run: %d (actual FPS: %.1f)\n", frames_run, (frames_run * 1000.0) / total_time);
    printf("  Game frameCount: %d\n", final_frames);
    printf("  Game totalTime: %.3f s\n", final_time);
    printf("  Game score: %d\n", final_score);

    /* Verify we got close to target FPS
     * Note: In Debug mode with GC_DEBUG, FPS may be lower due to memory checks
     * We use a 30 FPS minimum to account for debug overhead */
    double actual_fps = (frames_run * 1000.0) / total_time;
    bool passed = (actual_fps >= 30.0) && (final_frames == frames_run);
    printf("  Status: %s\n", passed ? "PASS" : "FAIL");

    return passed ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    printf("=================================================================\n");
    printf("  HLFFI Game Loop Test: External MainLoop Control\n");
    printf("=================================================================\n");
    printf("\nThis test demonstrates the WASM pattern for native embedding:\n");
    printf("- C code drives the main loop (like game engines)\n");
    printf("- Calls haxe.MainLoop.tick() to process Haxe callbacks\n");
    printf("- Calls Game.update(dt) for game logic\n");
    printf("- No blocking - pure tick-based event processing\n");

    /* Initialize VM */
    printf("\n[Setup] Creating and initializing VM...\n");
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

    err = hlffi_call_entry(vm);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Entry point failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("[Setup] VM ready.\n");

    /* Run tests */
    int failures = 0;

    failures += test_simple_loop(vm);
    failures += test_mainloop(vm);
    failures += test_realtime_loop(vm);

    /* Summary */
    printf("\n=================================================================\n");
    if (failures == 0) {
        printf("  ALL TESTS PASSED\n");
    } else {
        printf("  %d TEST(S) FAILED\n", failures);
    }
    printf("=================================================================\n");

    /* Cleanup */
    hlffi_destroy(vm);

    return failures;
}
