/**
 * HLFFI Debug API Test
 *
 * Standalone test for the HashLink debugger integration.
 * Supports THREADED, NON_THREADED, and MANUAL_THREAD modes.
 *
 * Usage:
 *   test_debug.exe [path_to_debug_test.hl] [options]
 *
 * Options:
 *   --wait              Wait for debugger to connect before running
 *   --non-threaded      Use NON_THREADED mode (default: THREADED)
 *   --manual-thread     Use NON_THREADED mode but run in a manually created C thread
 *   -n, --no-interactive  Run immediately without waiting for Enter key
 *
 * VSCode launch.json:
 *   {
 *     "name": "Attach to HLFFI Debug Test",
 *     "request": "attach",
 *     "type": "hl",
 *     "port": 6112,
 *     "cwd": "${workspaceFolder}/test"
 *   }
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>  /* for _beginthreadex */
#else
#include <unistd.h>
#include <pthread.h>
#endif

#include <hlffi.h>
#include <hl.h>  /* for hl_register_thread */
#include <stdio.h>
#include <string.h>

#define DEBUG_PORT 6112

#ifdef _WIN32
/* Exception handler to see what's killing us */
static LONG WINAPI exception_handler(PEXCEPTION_POINTERS info) {
    /* Only print for non-breakpoint exceptions when debugger is present */
    /* Breakpoints are expected to be handled by the debugger */
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT &&
        info->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) {
        printf("\n!!! EXCEPTION: 0x%08lX at 0x%p\n",
               info->ExceptionRecord->ExceptionCode,
               info->ExceptionRecord->ExceptionAddress);
        fflush(stdout);
    }

    /* Continue searching for handlers - let debugger handle it */
    return EXCEPTION_CONTINUE_SEARCH;
}

/* Exit handler to trace why the process exits */
static void exit_handler(void) {
    printf("\n>>> EXIT HANDLER CALLED <<<\n");
    printf("    IsDebuggerPresent: %s\n", IsDebuggerPresent() ? "yes" : "no");
    fflush(stdout);
}
#endif

/* ========== MANUAL THREAD MODE ========== */

/* Data passed to manual thread */
typedef struct {
    hlffi_vm* vm;
    volatile bool completed;
    hlffi_error_code result;
} manual_thread_data;

/* Thread function for manual thread mode */
#ifdef _WIN32
static unsigned __stdcall manual_thread_func(void* param)
#else
static void* manual_thread_func(void* param)
#endif
{
    manual_thread_data* data = (manual_thread_data*)param;

    printf("[Manual Thread] Started - registering with HashLink GC...\n");

    /* CRITICAL: Register this thread with HashLink GC before any HL calls */
    int stack_marker;
    hl_register_thread(&stack_marker);

    printf("[Manual Thread] Calling entry point...\n");
    data->result = hlffi_call_entry(data->vm);

    printf("[Manual Thread] Entry point returned: %s\n",
           hlffi_get_error_string(data->result));

    hl_unregister_thread();
    data->completed = true;

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main(int argc, char** argv) {
#ifdef _WIN32
    /* Install exception handler to catch what kills us */
    SetUnhandledExceptionFilter(exception_handler);
    AddVectoredExceptionHandler(0, exception_handler);  /* Low priority */
    atexit(exit_handler);  /* Called when process exits normally */
#endif

    printf("==============================================\n");
    printf("  HLFFI Debug API Test\n");
    printf("==============================================\n\n");

    printf("HLFFI Version: %s\n", hlffi_get_version());
    printf("HashLink Version: %s\n", hlffi_get_hl_version());
    printf("\n");

#ifdef HLFFI_HLC_MODE
    printf("ERROR: Debug API requires JIT mode!\n");
    printf("HLC mode uses native debuggers (Visual Studio, GDB).\n");
    return 1;
#endif

    /* Parse arguments */
    const char* hl_file = "..\\..\\..\\test\\debug_test.hl";
    bool wait_for_debugger = false;
    bool interactive = true;  /* Wait for user input before running */
    bool use_threaded_mode = true;  /* Default to THREADED mode */
    bool use_manual_thread = false; /* Run NON_THREADED in a manual C thread */

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--wait") == 0) {
            wait_for_debugger = true;
        } else if (strcmp(argv[i], "--no-interactive") == 0 || strcmp(argv[i], "-n") == 0) {
            interactive = false;
        } else if (strcmp(argv[i], "--non-threaded") == 0) {
            use_threaded_mode = false;
        } else if (strcmp(argv[i], "--manual-thread") == 0) {
            use_threaded_mode = false;
            use_manual_thread = true;
        } else if (argv[i][0] != '-') {
            hl_file = argv[i];
        }
    }

    printf("Bytecode file: %s\n", hl_file);
    printf("Debug port: %d\n", DEBUG_PORT);
    printf("Wait for debugger: %s\n", wait_for_debugger ? "yes" : "no");
    if (use_manual_thread) {
        printf("Integration mode: NON_THREADED (in manual C thread)\n");
    } else {
        printf("Integration mode: %s\n", use_threaded_mode ? "THREADED" : "NON_THREADED");
    }
    printf("\n");

    /* Create VM */
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        printf("ERROR: Failed to create VM\n");
        return 1;
    }

    /* Set integration mode BEFORE init */
    if (use_threaded_mode) {
        printf("Setting THREADED integration mode...\n");
        hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
    } else {
        printf("Using NON_THREADED integration mode (default)...\n");
        /* NON_THREADED is the default, but set explicitly for clarity */
        hlffi_set_integration_mode(vm, HLFFI_MODE_NON_THREADED);
    }

    /* Initialize VM */
    hlffi_error_code err = hlffi_init(vm, argc, argv);
    if (err != HLFFI_OK) {
        printf("ERROR: Failed to init VM: %s\n", hlffi_get_error_string(err));
        hlffi_destroy(vm);
        return 1;
    }

    /* Load bytecode */
    err = hlffi_load_file(vm, hl_file);
    if (err != HLFFI_OK) {
        printf("ERROR: Failed to load bytecode: %s\n", hlffi_get_error_string(err));
        printf("       %s\n", hlffi_get_error(vm));
        printf("\nMake sure to compile with debug info:\n");
        printf("  haxe --debug -hl debug_test.hl -main DebugTest\n");
        hlffi_destroy(vm);
        return 1;
    }

    printf("Bytecode loaded successfully.\n\n");

    /* Start debugger BEFORE starting thread */
    printf("Starting debugger on port %d...\n", DEBUG_PORT);

    err = hlffi_debug_start(vm, DEBUG_PORT, wait_for_debugger);
    if (err != HLFFI_OK) {
        printf("WARNING: Failed to start debugger: %s\n", hlffi_get_error_string(err));
        printf("         %s\n", hlffi_get_error(vm));
        printf("         Continuing without debugger...\n\n");
    } else {
        printf("Debugger server started!\n");
        if (wait_for_debugger) {
            printf("Waiting for debugger to connect...\n");
        } else if (interactive) {
            printf("\n");
            printf(">>> Attach VSCode debugger NOW <<<\n");
            printf("    1. Open VSCode with HashLink Debugger extension\n");
            printf("    2. Use 'Attach' configuration on port %d\n", DEBUG_PORT);
            printf("    3. Set breakpoints in DebugTest.hx\n");
            printf("    4. Press Enter to continue execution...\n");
            printf("\n");
            getchar();  /* Wait for user to attach debugger */
        } else {
            printf("Non-interactive mode - running immediately.\n");
        }
    }

    /* Check if debugger attached */
    if (hlffi_debug_is_attached(vm)) {
        printf("TCP debugger attached!\n");

#ifdef _WIN32
        printf("Waiting for native debugger (IsDebuggerPresent)...\n");
        int wait_count = 0;
        while (!IsDebuggerPresent() && wait_count < 100) {  /* 10 second timeout */
            Sleep(100);
            wait_count++;
        }
        if (IsDebuggerPresent()) {
            printf("Native debugger attached!\n");
            printf("Waiting 3 seconds for debugger to set breakpoints...\n");
            Sleep(3000);  /* Give VSCode time to set breakpoints */
            printf("Continuing execution. Breakpoints should work now.\n\n");
        } else {
            printf("WARNING: Native debugger not detected after 10 seconds.\n");
            printf("         Breakpoints may not work correctly.\n\n");
        }
#else
        printf("Waiting for debugger to set breakpoints...\n");
        usleep(2000000);
#endif
    } else {
        printf("No debugger attached (breakpoints won't pause execution).\n\n");
    }

    if (use_threaded_mode) {
        /* THREADED MODE: Start VM thread - this calls entry point in a separate thread */
        printf("Starting VM thread...\n");
        err = hlffi_thread_start(vm);
        if (err != HLFFI_OK) {
            printf("ERROR: Failed to start VM thread: %s\n", hlffi_get_error_string(err));
            printf("       %s\n", hlffi_get_error(vm));
            hlffi_debug_stop(vm);
            hlffi_destroy(vm);
            return 1;
        }
        printf("VM thread started - Haxe code is now running.\n\n");

        printf("========== Main thread waiting ==========\n");

        /* Wait for VM thread to complete */
        while (hlffi_thread_is_running(vm)) {
#ifdef _WIN32
            Sleep(100);
#else
            usleep(100000);
#endif
        }

        printf("========== VM thread completed ==========\n\n");

        /* Cleanup thread */
        printf("Stopping VM thread...\n");
        hlffi_thread_stop(vm);
    } else if (use_manual_thread) {
        /* MANUAL THREAD MODE: Run NON_THREADED in a manually created C thread */
        printf("Creating manual C thread for Haxe execution...\n\n");
        printf("========== Manual thread execution ==========\n");

        manual_thread_data thread_data = { vm, false, HLFFI_OK };

#ifdef _WIN32
        HANDLE thread_handle = (HANDLE)_beginthreadex(
            NULL, 0, manual_thread_func, &thread_data, 0, NULL);
        if (!thread_handle) {
            printf("ERROR: Failed to create thread\n");
            hlffi_debug_stop(vm);
            hlffi_destroy(vm);
            return 1;
        }

        /* Wait for thread to complete */
        while (!thread_data.completed) {
            Sleep(100);
        }

        WaitForSingleObject(thread_handle, INFINITE);
        CloseHandle(thread_handle);
#else
        pthread_t thread_handle;
        if (pthread_create(&thread_handle, NULL, manual_thread_func, &thread_data) != 0) {
            printf("ERROR: Failed to create thread\n");
            hlffi_debug_stop(vm);
            hlffi_destroy(vm);
            return 1;
        }

        pthread_join(thread_handle, NULL);
#endif

        printf("========== Manual thread completed ==========\n\n");
        err = thread_data.result;

        if (err != HLFFI_OK) {
            printf("ERROR: Entry point failed: %s\n", hlffi_get_error_string(err));
            printf("       %s\n", hlffi_get_error(vm));
        }
    } else {
        /* NON_THREADED MODE: Call entry point directly on this thread */
        printf("Calling entry point (NON_THREADED)...\n\n");
        printf("========== Haxe execution ==========\n");
        err = hlffi_call_entry(vm);
        printf("========== Execution completed ==========\n\n");

        if (err != HLFFI_OK) {
            printf("ERROR: Entry point failed: %s\n", hlffi_get_error_string(err));
            printf("       %s\n", hlffi_get_error(vm));
        }
    }

    /* Check debugger status after execution */
    if (hlffi_debug_is_attached(vm)) {
        printf("Debugger was attached during execution.\n");
    }

    /* Cleanup */

    printf("Stopping debugger...\n");
    hlffi_debug_stop(vm);

    printf("Destroying VM...\n");
    hlffi_destroy(vm);

    printf("\nDone.\n");
    return 0;
}
