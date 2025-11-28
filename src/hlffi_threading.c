/**
 * HLFFI Threading Implementation
 * Dedicated VM thread with message queue
 * Phase 1 implementation
 *
 * ARCHITECTURE:
 * - Main thread: Enqueues messages and waits for responses
 * - VM thread: Processes messages in a loop, calls into HashLink
 * - Message queue: Thread-safe circular buffer with mutex/condvar
 *
 * USAGE (Mode 2 - THREADED):
 *   hlffi_vm* vm = hlffi_create();
 *   hlffi_set_integration_mode(vm, HLFFI_MODE_THREADED);
 *   hlffi_init_args(vm, argc, argv);
 *   hlffi_load_file(vm, "game.hl");
 *   hlffi_thread_start(vm);  // Spawns thread, calls entry point
 *
 *   // From main thread (thread-safe)
 *   hlffi_thread_call_sync(vm, my_callback, userdata);
 *
 *   hlffi_thread_stop(vm);
 *   hlffi_destroy(vm);
 */

#include "hlffi_internal.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    typedef HANDLE pthread_t;
    typedef CRITICAL_SECTION pthread_mutex_t;
    typedef CONDITION_VARIABLE pthread_cond_t;
    #define pthread_mutex_init(m, a) InitializeCriticalSection(m)
    #define pthread_mutex_destroy(m) DeleteCriticalSection(m)
    #define pthread_mutex_lock(m) EnterCriticalSection(m)
    #define pthread_mutex_unlock(m) LeaveCriticalSection(m)
    #define pthread_cond_init(c, a) InitializeConditionVariable(c)
    #define pthread_cond_destroy(c) ((void)0)
    #define pthread_cond_wait(c, m) SleepConditionVariableCS(c, m, INFINITE)
    #define pthread_cond_signal(c) WakeConditionVariable(c)
    #define pthread_cond_broadcast(c) WakeAllConditionVariable(c)
#else
    #include <pthread.h>
#endif

/* ========== MESSAGE QUEUE ========== */

#define HLFFI_MSG_QUEUE_SIZE 256

typedef enum {
    HLFFI_MSG_NONE,
    HLFFI_MSG_CALL_SYNC,
    HLFFI_MSG_CALL_ASYNC,
    HLFFI_MSG_STOP
} hlffi_message_type;

typedef struct {
    hlffi_message_type type;
    hlffi_thread_func func;
    void* userdata;
    hlffi_thread_async_callback async_callback;
    void* result;
    bool completed;
} hlffi_thread_message;

typedef struct {
    hlffi_thread_message messages[HLFFI_MSG_QUEUE_SIZE];
    int head;  /* Read position */
    int tail;  /* Write position */
    int count; /* Number of messages */
} hlffi_thread_message_queue;

/* ========== QUEUE OPERATIONS (NOT THREAD-SAFE, MUST HOLD MUTEX) ========== */

static hlffi_thread_message_queue* queue_create(void) {
    hlffi_thread_message_queue* q = (hlffi_thread_message_queue*)calloc(1, sizeof(hlffi_thread_message_queue));
    return q;
}

static void queue_destroy(hlffi_thread_message_queue* q) {
    free(q);
}

static bool queue_is_empty(hlffi_thread_message_queue* q) {
    return q->count == 0;
}

static bool queue_is_full(hlffi_thread_message_queue* q) {
    return q->count >= HLFFI_MSG_QUEUE_SIZE;
}

static bool queue_enqueue(hlffi_thread_message_queue* q, hlffi_thread_message* msg) {
    if (queue_is_full(q)) {
        return false;
    }
    q->messages[q->tail] = *msg;
    q->tail = (q->tail + 1) % HLFFI_MSG_QUEUE_SIZE;
    q->count++;
    return true;
}

static bool queue_dequeue(hlffi_thread_message_queue* q, hlffi_thread_message* msg) {
    if (queue_is_empty(q)) {
        return false;
    }
    *msg = q->messages[q->head];
    q->head = (q->head + 1) % HLFFI_MSG_QUEUE_SIZE;
    q->count--;
    return true;
}

/* ========== THREAD MAIN LOOP ========== */

#ifdef _WIN32
static unsigned __stdcall vm_thread_main(void* param)
#else
static void* vm_thread_main(void* param)
#endif
{
    hlffi_vm* vm = (hlffi_vm*)param;
    pthread_mutex_t* mutex = (pthread_mutex_t*)vm->thread_mutex;
    pthread_cond_t* cond_var = (pthread_cond_t*)vm->thread_cond_var;
    pthread_cond_t* response_cond = (pthread_cond_t*)vm->thread_response_cond;
    hlffi_thread_message_queue* queue = (hlffi_thread_message_queue*)vm->message_queue;

    /* Call entry point (may block if Haxe has while loop) */
    hlffi_call_entry(vm);

    /* Process messages until stop requested */
    while (1) {
        hlffi_thread_message msg;
        bool has_message = false;

        /* Wait for message */
        pthread_mutex_lock(mutex);
        while (queue_is_empty(queue) && !vm->thread_should_stop) {
            pthread_cond_wait(cond_var, mutex);
        }

        if (vm->thread_should_stop && queue_is_empty(queue)) {
            pthread_mutex_unlock(mutex);
            break;
        }

        if (queue_dequeue(queue, &msg)) {
            has_message = true;
        }
        pthread_mutex_unlock(mutex);

        /* Process message */
        if (has_message) {
            if (msg.type == HLFFI_MSG_STOP) {
                break;
            } else if (msg.type == HLFFI_MSG_CALL_SYNC) {
                /* Execute function */
                if (msg.func) {
                    msg.func(vm, msg.userdata);
                }
                /* Signal completion */
                pthread_mutex_lock(mutex);
                msg.completed = true;
                /* Store result back in queue message for retrieval */
                for (int i = 0; i < HLFFI_MSG_QUEUE_SIZE; i++) {
                    if (&queue->messages[i] == &msg) {
                        queue->messages[i].completed = true;
                        break;
                    }
                }
                pthread_cond_signal(response_cond);
                pthread_mutex_unlock(mutex);
            } else if (msg.type == HLFFI_MSG_CALL_ASYNC) {
                /* Execute function */
                void* result = NULL;
                if (msg.func) {
                    msg.func(vm, msg.userdata);
                }
                /* Call async callback (on VM thread) */
                if (msg.async_callback) {
                    msg.async_callback(vm, result, msg.userdata);
                }
            }
        }
    }

    vm->thread_running = false;
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* ========== THREADING API ========== */

hlffi_error_code hlffi_thread_start(hlffi_vm* vm) {
    if (!vm) {
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (vm->integration_mode != HLFFI_MODE_THREADED) {
        snprintf(vm->error_msg, sizeof(vm->error_msg),
                 "Thread start requires THREADED mode (use hlffi_set_integration_mode)");
        return HLFFI_ERROR_THREAD_START_FAILED;
    }

    if (vm->thread_running) {
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Thread already running");
        return HLFFI_ERROR_THREAD_ALREADY_RUNNING;
    }

    if (!vm->module_loaded) {
        snprintf(vm->error_msg, sizeof(vm->error_msg),
                 "No module loaded (call hlffi_load_file first)");
        return HLFFI_ERROR_NOT_INITIALIZED;
    }

    /* Allocate threading resources */
    vm->thread_mutex = malloc(sizeof(pthread_mutex_t));
    vm->thread_cond_var = malloc(sizeof(pthread_cond_t));
    vm->thread_response_cond = malloc(sizeof(pthread_cond_t));
    vm->message_queue = queue_create();
    vm->thread_handle = malloc(sizeof(pthread_t));

    if (!vm->thread_mutex || !vm->thread_cond_var || !vm->thread_response_cond ||
        !vm->message_queue || !vm->thread_handle) {
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Failed to allocate threading resources");
        /* Cleanup partial allocation */
        free(vm->thread_mutex);
        free(vm->thread_cond_var);
        free(vm->thread_response_cond);
        queue_destroy((hlffi_thread_message_queue*)vm->message_queue);
        free(vm->thread_handle);
        return HLFFI_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize synchronization primitives */
    pthread_mutex_init((pthread_mutex_t*)vm->thread_mutex, NULL);
    pthread_cond_init((pthread_cond_t*)vm->thread_cond_var, NULL);
    pthread_cond_init((pthread_cond_t*)vm->thread_response_cond, NULL);

    /* Start thread */
    vm->thread_should_stop = false;
    vm->thread_running = true;

#ifdef _WIN32
    *(HANDLE*)vm->thread_handle = (HANDLE)_beginthreadex(NULL, 0, vm_thread_main, vm, 0, NULL);
    if (*(HANDLE*)vm->thread_handle == 0) {
#else
    if (pthread_create((pthread_t*)vm->thread_handle, NULL, vm_thread_main, vm) != 0) {
#endif
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Failed to create thread");
        vm->thread_running = false;
        pthread_mutex_destroy((pthread_mutex_t*)vm->thread_mutex);
        pthread_cond_destroy((pthread_cond_t*)vm->thread_cond_var);
        pthread_cond_destroy((pthread_cond_t*)vm->thread_response_cond);
        queue_destroy((hlffi_thread_message_queue*)vm->message_queue);
        free(vm->thread_mutex);
        free(vm->thread_cond_var);
        free(vm->thread_response_cond);
        free(vm->thread_handle);
        vm->thread_mutex = NULL;
        vm->thread_cond_var = NULL;
        vm->thread_response_cond = NULL;
        vm->message_queue = NULL;
        vm->thread_handle = NULL;
        return HLFFI_ERROR_OUT_OF_MEMORY;
    }

    return HLFFI_OK;
}

hlffi_error_code hlffi_thread_stop(hlffi_vm* vm) {
    if (!vm) {
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (!vm->thread_running) {
        return HLFFI_OK;  /* Already stopped */
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)vm->thread_mutex;
    pthread_cond_t* cond_var = (pthread_cond_t*)vm->thread_cond_var;
    hlffi_thread_message_queue* queue = (hlffi_thread_message_queue*)vm->message_queue;

    /* Send stop message */
    pthread_mutex_lock(mutex);
    vm->thread_should_stop = true;
    hlffi_thread_message msg = { .type = HLFFI_MSG_STOP };
    queue_enqueue(queue, &msg);
    pthread_cond_signal(cond_var);
    pthread_mutex_unlock(mutex);

    /* Wait for thread to exit */
#ifdef _WIN32
    WaitForSingleObject(*(HANDLE*)vm->thread_handle, INFINITE);
    CloseHandle(*(HANDLE*)vm->thread_handle);
#else
    pthread_join(*(pthread_t*)vm->thread_handle, NULL);
#endif

    /* Cleanup resources */
    pthread_mutex_destroy(mutex);
    pthread_cond_destroy(cond_var);
    pthread_cond_destroy((pthread_cond_t*)vm->thread_response_cond);
    queue_destroy(queue);
    free(vm->thread_mutex);
    free(vm->thread_cond_var);
    free(vm->thread_response_cond);
    free(vm->thread_handle);
    vm->thread_mutex = NULL;
    vm->thread_cond_var = NULL;
    vm->thread_response_cond = NULL;
    vm->message_queue = NULL;
    vm->thread_handle = NULL;
    vm->thread_running = false;

    return HLFFI_OK;
}

bool hlffi_thread_is_running(hlffi_vm* vm) {
    if (!vm) {
        return false;
    }
    return vm->thread_running;
}

hlffi_error_code hlffi_thread_call_sync(hlffi_vm* vm, hlffi_thread_func func, void* userdata) {
    if (!vm || !func) {
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (!vm->thread_running) {
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Thread not running");
        return HLFFI_ERROR_THREAD_NOT_STARTED;
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)vm->thread_mutex;
    pthread_cond_t* cond_var = (pthread_cond_t*)vm->thread_cond_var;
    pthread_cond_t* response_cond = (pthread_cond_t*)vm->thread_response_cond;
    hlffi_thread_message_queue* queue = (hlffi_thread_message_queue*)vm->message_queue;

    /* Enqueue message */
    hlffi_thread_message msg = {
        .type = HLFFI_MSG_CALL_SYNC,
        .func = func,
        .userdata = userdata,
        .completed = false
    };

    pthread_mutex_lock(mutex);

    if (queue_is_full(queue)) {
        pthread_mutex_unlock(mutex);
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Message queue full");
        return HLFFI_ERROR_OUT_OF_MEMORY;
    }

    /* Store message index for tracking completion */
    int msg_index = queue->tail;
    queue_enqueue(queue, &msg);
    pthread_cond_signal(cond_var);

    /* Wait for completion */
    while (!queue->messages[msg_index].completed) {
        pthread_cond_wait(response_cond, mutex);
    }

    /* Clear completed flag */
    queue->messages[msg_index].completed = false;
    pthread_mutex_unlock(mutex);

    return HLFFI_OK;
}

hlffi_error_code hlffi_thread_call_async(
    hlffi_vm* vm,
    hlffi_thread_func func,
    hlffi_thread_async_callback callback,
    void* userdata
) {
    if (!vm || !func) {
        return HLFFI_ERROR_INVALID_ARGUMENT;
    }

    if (!vm->thread_running) {
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Thread not running");
        return HLFFI_ERROR_THREAD_NOT_STARTED;
    }

    pthread_mutex_t* mutex = (pthread_mutex_t*)vm->thread_mutex;
    pthread_cond_t* cond_var = (pthread_cond_t*)vm->thread_cond_var;
    hlffi_thread_message_queue* queue = (hlffi_thread_message_queue*)vm->message_queue;

    /* Enqueue message */
    hlffi_thread_message msg = {
        .type = HLFFI_MSG_CALL_ASYNC,
        .func = func,
        .userdata = userdata,
        .async_callback = callback,
        .completed = false
    };

    pthread_mutex_lock(mutex);

    if (queue_is_full(queue)) {
        pthread_mutex_unlock(mutex);
        snprintf(vm->error_msg, sizeof(vm->error_msg), "Message queue full");
        return HLFFI_ERROR_OUT_OF_MEMORY;
    }

    queue_enqueue(queue, &msg);
    pthread_cond_signal(cond_var);
    pthread_mutex_unlock(mutex);

    return HLFFI_OK;
}

/* ========== WORKER THREAD HELPERS ========== */

void hlffi_worker_register(void) {
    /* Register current thread with HashLink GC */
    int stack_marker;
    hl_register_thread(&stack_marker);
}

void hlffi_worker_unregister(void) {
    /* Unregister current thread from HashLink GC */
    hl_unregister_thread();
}

/* Note: hlffi_blocking_begin and hlffi_blocking_end are implemented in hlffi_callbacks.c */
