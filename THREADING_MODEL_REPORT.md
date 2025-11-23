# HashLink VM Threading Model & FFI Design Implications

**Investigation Date**: 2025-11-22
**Critical Question**: Does the HL VM consume/block the thread it runs on? Should the FFI run the VM on a separate thread?
**Answer**: ⚠️ **NUANCED** - The VM itself does NOT block, but standard Heaps applications DO block in their main loop

---

## Executive Summary

HashLink VM has a **dual-personality threading model** that depends on the integration pattern:

1. **VM Core**: Does NOT block - `hl_call0(entry)` returns control immediately
2. **Standard Applications** (Heaps/SDL): DO block - `runMainLoop()` contains blocking while loop
3. **Embedded Integration** (Engines): Should NOT block - host controls main thread and calls update functions

**Critical Insight for FFI Design**:
- ✅ **DO**: Keep VM on main thread, host controls frame loop
- ❌ **DON'T**: Let standard Heaps `runMainLoop()` execute (it blocks!)
- ✅ **DO**: Call the registered loop function directly from host's tick/update

---

## Part 1: VM Thread Consumption Analysis

### Does hl_module_init() or entry point block the calling thread?

**Answer**: ❌ **NO** - Both return control immediately

**Evidence from `src/main.c`** ([source](https://github.com/HaxeFoundation/hashlink/blob/master/src/main.c)):

```c
// main.c lines 197-315
int main(int argc, char *argv[]) {
    // ... setup code ...

    // Initialize module (does NOT block)
    if( !hl_module_init(ctx.m, &ctx.boot_fun, true) )
        return 1;

    // Call entry point (does NOT block)
    ctx.ret = hl_dyn_call_safe(&cl, NULL, 0, &isExc);

    // CONTINUES EXECUTING after entry point returns!
    if( isExc ) {
        // Handle exception
        hl_debug_break();
    }

    // Cleanup and exit
    hl_module_free(ctx.m);
    hl_free(&ctx.code->alloc);
    hl_global_free();
    return isExc ? 1 : 0;
}
```

**Execution Flow**:
```
main()
  → hl_module_init() [RETURNS]
  → hl_dyn_call_safe(entry) [RETURNS]
  → cleanup
  → exit(0)
```

**Verdict**: ✅ VM initialization and entry point do NOT consume the calling thread

---

### Can you return control to the host thread after initialization?

**Answer**: ✅ **YES** - Control returns immediately after entry point

**BUT** there's a critical caveat for game frameworks...

---

### How do game loops work in HL - does the VM maintain control?

**Answer**: ⚠️ **DEPENDS ON INTEGRATION PATTERN**

#### Pattern A: Standalone Heaps Application (BLOCKS)

**Source**: `hxd/System.hl.hx` ([GitHub](https://github.com/HeapsIO/heaps/blob/master/hxd/System.hl.hx))

```haxe
// System.hl.hx
static var loopFunc : Void -> Void;

public static function setLoop(f : Void -> Void) : Void {
    loopFunc = f;
}

static function runMainLoop() {
    // THIS BLOCKS IN A WHILE LOOP!
    while( isAlive() ) {
        @:privateAccess try {
            haxe.MainLoop.tick();
            mainLoop();
        } catch(e:Dynamic) {
            handleError(e);
        }
    }
}

// Entry point pattern
function main() {
    new MyApp();  // Calls setLoop(mainLoop)
    haxe.Timer.delay(runMainLoop, 0);  // STARTS BLOCKING LOOP
}
```

**Execution Flow for Standalone App**:
```
main()
  → hl_call0(entry)
    → Main.main()
      → new hxd.App()
        → setLoop(app.mainLoop)
        → Timer.delay(runMainLoop, 0)
          → runMainLoop() BLOCKS IN WHILE LOOP ← Thread consumed here!
```

**Verdict for Standalone**: ❌ Standard Heaps apps DO block the thread in `runMainLoop()`

---

#### Pattern B: Embedded Integration (DOES NOT BLOCK)

**For engine integration**, you bypass `runMainLoop()` entirely:

```c
// Engine-controlled pattern
void engine_init() {
    hlffi_vm* vm = hlffi_create();
    hlffi_call_entry(vm);  // Sets up loop function, RETURNS

    // Lookup the registered loop function
    update_func = hlffi_lookup(vm, "mainLoop", 0);
}

void engine_tick() {
    // Engine controls when to call HL code
    hlffi_call0(update_func);  // Call game logic, RETURNS immediately

    // Engine continues with rendering, physics, etc.
    render_frame();
    update_physics();
}
```

**Execution Flow for Embedded**:
```
Engine Init:
  hlffi_create()
    → hl_call0(entry)
      → Main.main()
        → setLoop(mainLoop)  // Registers function
      → RETURNS to engine  ← Control returned!

Engine Tick:
  hlffi_call0(mainLoop)  ← Engine calls when ready
    → update()
    → RETURNS to engine  ← Control returned!
```

**Verdict for Embedded**: ✅ VM does NOT block when host controls the loop

---

### SDL Event Loop Confirmation

**Source**: `libs/sdl/sdl.c` ([GitHub](https://github.com/HaxeFoundation/hashlink/blob/master/libs/sdl/sdl.c))

```c
HL_PRIM bool HL_NAME(event_loop)(event_data *event) {
    while (true) {
        SDL_Event e;
        if (SDL_PollEvent(&e) == 0) break;  // Non-blocking poll

        // Process one event
        switch (e.type) {
            case SDL_MOUSEMOTION: /* ... */ return true;
            case SDL_KEYDOWN: /* ... */ return true;
            // ...
        }
    }
    return false;  // No events, returns immediately
}
```

**Key Points**:
- Uses `SDL_PollEvent()` (non-blocking)
- Processes one event and returns
- Does NOT call `SDL_WaitEvent()` (which would block)
- Host controls when to poll for events

---

## Part 2: Thread Registration & Safety

### What does hl_register_thread() actually do?

**Answer**: Registers thread with GC for stack scanning

**Source**: `src/std/thread.c` ([GitHub](https://github.com/HaxeFoundation/hashlink/blob/master/src/std/thread.c))

```c
static void gc_thread_entry(thread_start *_s) {
    thread_start s = *_s;
    hl_register_thread(&s);  // Register with GC
    hl_lock_release(_s->wait);
    s.wait = _s->wait = NULL;
    _s = NULL;
    s.callb(s.param);
    hl_unregister_thread();  // Cleanup when thread exits
}
```

**What it does**:
1. **Saves stack boundary** - `&s` provides stack top address for GC
2. **Adds thread to GC tracking** - So GC can scan this thread's stack
3. **Required for ALL threads** - Any thread calling HL code must register

**From Issue #42** ([Multithreading Support](https://github.com/HaxeFoundation/hashlink/issues/42)):
> "Add a global lock for gc_alloc_gen to prevent two threads from allocating at the same time. Keep track of all threads currently started and their stack addresses. When doing a GC collection, we need to stop all threads and scan their stack + registers."

**Stack Top Requirement** (from Issue #752):
> "The stack_top must remain valid during GC. Using a local variable in init function causes dangling pointer when function returns."

**Correct pattern**:
```c
// WRONG - stack variable goes out of scope
void init_plugin() {
    void *stack_top = NULL;
    hl_register_thread(&stack_top);  // ⚠️ Dangling pointer after return!
}

// RIGHT - persistent storage
static void *g_stack_top = NULL;
void init_plugin() {
    hl_register_thread(&g_stack_top);
}

// BETTER - NULL for main thread
void main_thread_init() {
    hl_register_thread(NULL);  // HL handles it for main thread
}
```

---

### Can you call HL functions from different threads?

**Answer**: ✅ **YES**, but with requirements:

**Requirements**:
1. Thread must be registered with `hl_register_thread()`
2. If calling from registered thread, `hl_dyn_call()` is thread-safe
3. GC may stop all threads during collection (performance impact)

**From Issue #42** ([source](https://github.com/HaxeFoundation/hashlink/issues/42)):
> "Performance: Allocations are roughly 2.25x slower with threading enabled (1.52s vs 0.67s for 40 million allocations) due to mutex contention."

**Thread Safety Pattern**:
```c
// Worker thread calling into HL
void worker_thread(void* param) {
    // 1. Register thread
    static void *stack_top = NULL;  // Must be persistent!
    hl_register_thread(&stack_top);

    // 2. Call HL function
    vclosure *hl_callback = (vclosure*)param;
    hl_dyn_call(hl_callback, NULL, 0);  // Thread-safe

    // 3. Unregister before exit
    hl_unregister_thread();
}
```

---

### What are the thread-safety requirements for hl_dyn_call()?

**Answer**: Caller thread must be registered, GC-safe

**Thread Safety Guarantees**:
- ✅ `hl_dyn_call()` itself is thread-safe
- ✅ Allocations use mutex when threading enabled
- ⚠️ GC will stop ALL threads during collection
- ❌ Calling from unregistered thread → fatal error

**Error from Issue #188** ([source](https://github.com/HaxeFoundation/hashlink/issues/188)):
```
FATAL ERROR : Can't lock GC in unregistered thread
```

**GC Thread Safety** (from [GC Blog](https://haxe.org/blog/hashlink-gc/)):
> "The GC scans all threads' C stacks and registers. All threads must be registered so the collector knows where to look for roots."

---

### Look at hl_blocking() and its purpose

**Answer**: Notifies GC that thread is in external blocking operation

**From Issue #188** ([source](https://github.com/HaxeFoundation/hashlink/issues/188)):
> "For multithreading, you can use `hl_blocking(true); .... hl_blocking(false);` with the requirement that you should not perform any allocation between the two calls. This is an optimization that prevents other threads from waiting for your blocking operation in case they want to perform a GC collection."

**Use Case**: Calling external code that might block (I/O, sleep, etc.)

**Pattern**:
```c
// Calling blocking external function from HL callback
HL_PRIM void HL_NAME(call_external_api)(vstring *url) {
    char *c_url = hl_to_utf8(url);

    // Tell GC we're leaving HL scope
    hl_blocking(true);

    // External blocking call (network I/O)
    http_response *resp = curl_get(c_url);  // May take seconds!

    // Back in HL scope
    hl_blocking(false);

    // Now safe to allocate HL objects
    process_response(resp);
}
```

**Why it matters**:
- GC needs to stop all threads during collection
- If thread is blocked in external code, GC shouldn't wait for it
- `hl_blocking(true)` tells GC "don't wait for me"
- **MUST** be balanced - `hl_blocking(false)` required

**Error from Issue #188**:
```
FATAL ERROR : Can't lock GC in hl_blocking section
```
Caused by calling `hl_blocking(true)` while already in blocking state (unbalanced calls).

---

## Part 3: Engine Integration Patterns

### Pattern 1: Main Thread Integration (RECOMMENDED)

**Use When**: Embedding in game engine (Unreal, Unity, Godot, etc.)

**Architecture**:
```
┌─────────────────────────────────────┐
│  Game Engine (C++)                  │
│  ┌──────────────────────────────┐   │
│  │  Main Thread                 │   │
│  │  ┌────────────────────────┐  │   │
│  │  │  Engine Tick Loop      │  │   │
│  │  │  ├─ HL Update          │  │   │  ← Host controls
│  │  │  ├─ Physics            │  │   │
│  │  │  ├─ Rendering          │  │   │
│  │  │  └─ Present            │  │   │
│  │  └────────────────────────┘  │   │
│  └──────────────────────────────┘   │
│                                     │
│  HashLink VM                        │
│  ├─ No separate thread              │
│  ├─ Called from engine tick         │
│  └─ Returns control immediately     │
└─────────────────────────────────────┘
```

**Implementation**:
```c
// Engine initialization
void UMyGameMode::BeginPlay() {
    // 1. Initialize HL VM on main thread
    vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // Returns immediately

    // 2. Lookup update function
    update_func = hlffi_lookup(vm, "Game.update", 1);

    // Entry point has returned - engine continues
}

// Engine tick (called every frame by Unreal)
void UMyGameMode::Tick(float DeltaTime) {
    // 3. Call HL update function
    vdynamic *dt_arg = hl_alloc_dynamic(&hlt_f32);
    dt_arg->v.f = DeltaTime;

    hlffi_call1(update_func, dt_arg);  // Returns immediately

    // 4. Engine continues with other systems
    UpdatePhysics();
    UpdateAI();
    // Unreal handles rendering
}
```

**Advantages**:
- ✅ Simplest integration - no thread synchronization
- ✅ Deterministic execution order
- ✅ Low overhead - no context switching
- ✅ Easy debugging - single-threaded
- ✅ Engine controls framerate and timing

**Disadvantages**:
- ⚠️ HL code blocks engine if slow (must be fast!)
- ⚠️ Can't use HL for async operations easily

**Verdict**: ✅ **RECOMMENDED** for most engine integrations

---

### Pattern 2: Dedicated Thread (ADVANCED)

**Use When**: HL code needs to run independently of engine framerate

**Architecture**:
```
┌─────────────────────────────────────────────────┐
│  Game Engine (C++)                              │
│  ┌──────────────────┐   ┌──────────────────┐   │
│  │  Main Thread     │   │  HL Thread       │   │
│  │  ├─ Physics      │   │  ├─ HL Update    │   │
│  │  ├─ Rendering    │   │  ├─ Game Logic   │   │
│  │  └─ Present      │   │  └─ AI           │   │
│  └────────┬─────────┘   └─────────┬────────┘   │
│           │                       │            │
│           └───── Shared Data ─────┘            │
│              (Mutex/Atomic)                    │
└─────────────────────────────────────────────────┘
```

**Implementation**:
```c
// HL thread state
typedef struct {
    hlffi_vm *vm;
    vclosure *update_func;
    std::atomic<bool> running;
    std::mutex state_mutex;
    GameState shared_state;
} hl_thread_context;

// HL worker thread
void hl_worker_thread(hl_thread_context *ctx) {
    // 1. Register THIS thread with HL
    static void *stack_top = NULL;
    hl_register_thread(&stack_top);

    // 2. Run HL update loop
    while (ctx->running.load()) {
        // Call HL update
        hl_dyn_call(ctx->update_func, NULL, 0);

        // Synchronize shared state
        {
            std::lock_guard<std::mutex> lock(ctx->state_mutex);
            // Copy data to/from engine
            ctx->shared_state.player_pos = get_hl_player_pos();
        }

        // Sleep to avoid spinning
        std::this_thread::sleep_for(16ms);  // ~60fps
    }

    // 3. Cleanup
    hl_unregister_thread();
}

// Engine tick
void UMyGameMode::Tick(float DeltaTime) {
    // Read shared state from HL thread
    std::lock_guard<std::mutex> lock(hl_ctx.state_mutex);
    UpdatePlayerFromHL(hl_ctx.shared_state.player_pos);

    // Engine rendering continues
}
```

**Advantages**:
- ✅ HL can run at different framerate than engine
- ✅ Doesn't block engine rendering
- ✅ Good for AI or simulation code

**Disadvantages**:
- ❌ Complex - requires careful synchronization
- ❌ Data races if not careful with mutexes
- ❌ GC overhead - must coordinate collections
- ❌ Harder to debug

**Verdict**: ⚠️ **ADVANCED** - Only use if you need independent framerates

---

### Pattern 3: Hybrid (Main + Worker Threads)

**Use When**: HL for gameplay on main thread, HL for async I/O on workers

**Architecture**:
```
┌────────────────────────────────────────────────────┐
│  Engine Main Thread                                │
│  ├─ HL Gameplay Update (main thread)               │
│  ├─ Physics                                        │
│  └─ Rendering                                      │
│                                                    │
│  Worker Threads (HL-registered)                    │
│  ├─ Thread 1: HL Network I/O                       │
│  ├─ Thread 2: HL Asset Loading                     │
│  └─ Thread 3: HL AI Pathfinding                    │
└────────────────────────────────────────────────────┘
```

**Implementation**:
```c
// Worker thread for async operations
void hl_network_thread(hlffi_vm *vm) {
    // Register thread
    static void *stack_top = NULL;
    hl_register_thread(&stack_top);

    while (running) {
        // Get HL callback from queue
        vclosure *callback = pop_network_callback();

        // Perform blocking I/O with hl_blocking
        hl_blocking(true);
        http_response *resp = curl_get(url);
        hl_blocking(false);

        // Call HL callback with result
        hl_dyn_call(callback, &resp_arg, 1);
    }

    hl_unregister_thread();
}

// Main thread still calls HL update
void Tick(float dt) {
    hlffi_call1(update_func, dt_arg);  // Main gameplay logic
}
```

**Verdict**: ✅ **RECOMMENDED** for complex games with async operations

---

### How do existing game engines (Heaps.io, etc.) handle the main loop?

**Heaps.io Pattern** (from `hxd/App.hx` and `System.hl.hx`):

```haxe
// hxd/App.hx
class App {
    function mainLoop() {
        hxd.Timer.update();
        sevents.checkEvents();  // Process input
        update(hxd.Timer.dt);   // User game logic
        engine.render(this);    // Render frame
    }

    static function main() {
        new MyApp();
    }
}

// hxd/System.hl.hx
static function runMainLoop() {
    while (isAlive()) {
        @:privateAccess try {
            haxe.MainLoop.tick();
            mainLoop();  // Call registered loop function
        } catch(e:Dynamic) {
            handleError(e);
        }
    }
}
```

**Execution Flow**:
1. `Main.main()` creates `new MyApp()`
2. `MyApp` constructor calls `hxd.System.setLoop(mainLoop)`
3. `hxd.System.start()` calls `haxe.Timer.delay(runMainLoop, 0)`
4. `runMainLoop()` **blocks in while loop**, repeatedly calling `mainLoop()`

**Key Point**: ✅ Heaps uses **Haxe-controlled blocking loop** - VM takes over thread

**For embedding**: ❌ Don't use this pattern! Use Pattern 1 (Main Thread Integration) instead.

---

### Does the host application or HL control the main thread?

**Answer**: ⚠️ **DEPENDS ON INTEGRATION**

| Pattern | Thread Control | Entry Point Behavior | Main Loop Driver |
|---------|---------------|---------------------|------------------|
| **Standalone Heaps App** | HL controls | Returns, then blocks in `runMainLoop()` | Haxe (`while` loop) |
| **Embedded (Pattern 1)** | Host controls | Returns immediately | Host (calls HL each frame) |
| **Embedded (Pattern 2)** | Both | Returns immediately | Separate threads |

**For HLFFI v3.0**: ✅ **Host controls main thread** (Pattern 1)

---

## Part 4: Callback Threading

### When HL calls a C callback, what thread is it on?

**Answer**: ✅ **Same thread that called the HL function**

**Example Flow**:
```
Engine Main Thread:
  Tick()
    → hl_dyn_call(hl_update_func)
      → Haxe: update()
        → Haxe: callNativeCallback()
          → C: my_native_callback()  ← SAME THREAD (main)
        → Returns
      → Returns
    → Continues
```

**Implications**:
- ✅ Callbacks execute on predictable thread
- ✅ If called from main thread, callback is on main thread
- ✅ If called from worker thread, callback is on worker thread
- ⚠️ Callback must be fast if on main thread!

---

### Can C code call back into HL from a different thread?

**Answer**: ✅ **YES**, but thread must be registered

**Pattern**:
```c
// C callback registered from HL
static vclosure *g_hl_callback = NULL;

// Called from HL (main thread) to register callback
HL_PRIM void HL_NAME(register_callback)(vclosure *cb) {
    if (g_hl_callback) hl_remove_root(&g_hl_callback);
    g_hl_callback = cb;
    if (cb) hl_add_root(&g_hl_callback);  // Prevent GC
}

// Worker thread calls back into HL
void worker_thread(void *param) {
    // 1. MUST register thread
    static void *stack_top = NULL;
    hl_register_thread(&stack_top);

    // 2. Perform work with hl_blocking
    hl_blocking(true);
    do_expensive_io();  // External blocking operation
    hl_blocking(false);

    // 3. Call HL callback (now safe!)
    if (g_hl_callback) {
        hl_dyn_call(g_hl_callback, NULL, 0);
    }

    // 4. Cleanup
    hl_unregister_thread();
}
```

**Requirements**:
1. ✅ Thread MUST call `hl_register_thread()` first
2. ✅ Use `hl_blocking()` around external blocking operations
3. ✅ Unregister thread before exit
4. ⚠️ GC may stop all threads - performance impact

---

### What happens if you call hl_dyn_call() from a non-registered thread?

**Answer**: ❌ **FATAL ERROR**

**Error Message**:
```
FATAL ERROR : Can't lock GC in unregistered thread
```

**Why it fails**:
- GC needs to scan all thread stacks for roots
- Unregistered thread = GC doesn't know its stack boundary
- Potential for missed roots → memory corruption

**Solution**: Always register threads!

```c
// WRONG - crashes
void my_thread() {
    hl_dyn_call(callback, NULL, 0);  // ⚠️ FATAL ERROR
}

// RIGHT - safe
void my_thread() {
    static void *stack_top = NULL;
    hl_register_thread(&stack_top);
    hl_dyn_call(callback, NULL, 0);  // ✅ Works
    hl_unregister_thread();
}
```

---

## Part 5: Challenges and Solutions

### Challenge 1: VM Blocking in Standalone Apps

**Problem**: Standard Heaps apps block in `runMainLoop()` while loop

**Solution for Embedding**: Bypass `runMainLoop()`, call loop function directly

```c
// Don't let runMainLoop execute!
// Instead, lookup and call the mainLoop function yourself:

// In init:
vclosure *main_loop = hlffi_lookup(vm, "mainLoop", 0);

// In engine tick:
hlffi_call0(main_loop);  // Returns immediately
```

---

### Challenge 2: GC Thread Overhead

**Problem**: Multi-threaded GC 2.25x slower than single-threaded (from Issue #42)

**Solutions**:
1. **Minimize threads** - Use main thread pattern when possible
2. **Batch allocations** - Allocate in bursts, not per-frame
3. **Use hl_blocking()** - Tell GC when threads are blocked
4. **Consider HL/C mode** - Compiled mode has lower GC overhead

---

### Challenge 3: Callback Lifetime Management

**Problem**: HL closures can be GC'd if not rooted

**Solution**: Use GC roots for stored callbacks

```c
static vclosure *stored_callback = NULL;

void set_callback(vclosure *cb) {
    // Remove old root
    if (stored_callback) {
        hl_remove_root(&stored_callback);
    }

    // Store new callback
    stored_callback = cb;

    // Add new root (prevents GC)
    if (cb) {
        hl_add_root(&stored_callback);
    }
}

void clear_callback() {
    if (stored_callback) {
        hl_remove_root(&stored_callback);
        stored_callback = NULL;
    }
}
```

---

### Challenge 4: Stack Top Lifetime

**Problem**: Stack variable used for `hl_register_thread()` goes out of scope

**Solution**: Use static/global storage or NULL for main thread

```c
// WRONG
void init() {
    void *stack_top = NULL;
    hl_register_thread(&stack_top);  // ⚠️ Dangling after return!
}

// RIGHT - static storage
void init() {
    static void *stack_top = NULL;
    hl_register_thread(&stack_top);  // ✅ Persistent
}

// BETTER - NULL for main thread
void init() {
    hl_register_thread(NULL);  // ✅ HL handles main thread
}
```

---

### Challenge 5: Unbalanced hl_blocking() Calls

**Problem**: Calling `hl_blocking(true)` twice without `false` crashes

**Solution**: RAII wrapper for C++, careful manual management for C

```cpp
// C++ RAII wrapper
class HLBlockingGuard {
public:
    HLBlockingGuard() { hl_blocking(true); }
    ~HLBlockingGuard() { hl_blocking(false); }
};

void call_external() {
    HLBlockingGuard guard;  // Auto-balances
    curl_get("http://example.com");
    // hl_blocking(false) called automatically
}
```

```c
// C manual pattern
void call_external() {
    hl_blocking(true);

    // IMPORTANT: Even if error, must call hl_blocking(false)
    http_response *resp = curl_get(url);

    hl_blocking(false);  // Always call, even on error path!

    if (!resp) {
        // Handle error AFTER hl_blocking(false)
        return;
    }

    process_response(resp);
}
```

---

## Part 6: Recommended Approach for Engine Integration

### For Unreal Engine (and similar)

**Architecture**: Main Thread Integration (Pattern 1)

```cpp
// MyHaxeComponent.h
UCLASS()
class UMyHaxeComponent : public UActorComponent {
    GENERATED_BODY()

private:
    hlffi_vm *vm;
    void *update_func;
    void *render_func;

public:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ...) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};

// MyHaxeComponent.cpp
void UMyHaxeComponent::BeginPlay() {
    Super::BeginPlay();

    // 1. Initialize HL VM on main thread (Game Thread)
    vm = hlffi_create();
    hlffi_init(vm, 0, nullptr);

    // 2. Load bytecode
    FString BytecodePath = FPaths::ProjectContentDir() / TEXT("Scripts/game.hl");
    hlffi_load_file(vm, TCHAR_TO_UTF8(*BytecodePath));

    // 3. Call entry point (initializes game, returns immediately)
    hlffi_call_entry(vm);

    // 4. Lookup functions we'll call each frame
    update_func = hlffi_lookup(vm, "Game.update", 1);
    render_func = hlffi_lookup(vm, "Game.preRender", 0);

    UE_LOG(LogTemp, Log, TEXT("Haxe VM initialized"));
}

void UMyHaxeComponent::TickComponent(float DeltaTime,
                                      ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!update_func) return;

    // 5. Call HL update function (returns immediately)
    vdynamic *dt = hl_alloc_dynamic(&hlt_f32);
    dt->v.f = DeltaTime;
    hlffi_call1(update_func, dt);

    // 6. Pre-render callback
    if (render_func) {
        hlffi_call0(render_func);
    }

    // 7. Unreal continues with rendering, physics, etc.
}

void UMyHaxeComponent::EndPlay(const EEndPlayReason::Type Reason) {
    Super::EndPlay(Reason);

    // Cleanup (WARNING: can only do this once per process!)
    hlffi_destroy(vm);
    vm = nullptr;
}
```

**Haxe Side**:
```haxe
// Game.hx
class Game {
    static var instance:Game;

    static function main() {
        // DON'T start standard Heaps runMainLoop!
        // Just create the game instance
        instance = new Game();
    }

    // Called by Unreal every frame
    public static function update(dt:Float):Void {
        if (instance != null) {
            instance.updateGame(dt);
        }
    }

    public static function preRender():Void {
        if (instance != null) {
            instance.prepareRender();
        }
    }

    function updateGame(dt:Float):Void {
        // Your game logic here
        updatePlayer(dt);
        updateEnemies(dt);
        updatePhysics(dt);
    }

    function prepareRender():Void {
        // Pre-render setup
        updateAnimations();
        cullObjects();
    }
}
```

---

### Thread-Safety Checklist for Engine Integration

✅ **DO**:
- Use main thread integration (Pattern 1) by default
- Call `hl_register_thread(NULL)` on main thread during init
- Keep HL update functions fast (< 16ms for 60fps)
- Use GC roots (`hl_add_root`) for stored callbacks
- Call `hl_blocking()` around external I/O from HL code

❌ **DON'T**:
- Let `runMainLoop()` execute (it blocks!)
- Call HL from unregistered threads
- Forget to balance `hl_blocking(true)` / `hl_blocking(false)`
- Store closures without adding GC roots
- Use local variables for `stack_top` in `hl_register_thread()`

⚠️ **ADVANCED** (only if needed):
- Dedicated HL thread (Pattern 2) - requires careful synchronization
- Calling HL from worker threads - must register each thread
- Hot reload - experimental, use with caution

---

## Part 7: Code Examples of Proper Threading Patterns

### Example 1: Simple Main Thread Integration

```c
// simple_embed.c
#include "hlffi.h"

typedef struct {
    hlffi_vm *vm;
    void *update_func;
} game_state;

game_state g_game;

bool game_init(const char *bytecode_path) {
    // 1. Create VM (registers main thread internally)
    g_game.vm = hlffi_create();
    if (!g_game.vm) return false;

    // 2. Load and initialize
    hlffi_init(g_game.vm, 0, NULL);
    hlffi_load_file(g_game.vm, bytecode_path);
    hlffi_call_entry(g_game.vm);  // Returns immediately

    // 3. Lookup update function
    g_game.update_func = hlffi_lookup(g_game.vm, "update", 1);

    return g_game.update_func != NULL;
}

void game_tick(float delta_time) {
    // 4. Call HL update (returns immediately)
    vdynamic *dt = hl_alloc_dynamic(&hlt_f32);
    dt->v.f = delta_time;
    hlffi_call1(g_game.update_func, dt);
}

void game_shutdown() {
    // 5. Cleanup (only safe at process exit!)
    hlffi_destroy(g_game.vm);
}

// Main game loop (host controls thread)
int main(int argc, char **argv) {
    if (!game_init("game.hl")) {
        fprintf(stderr, "Failed to init game\n");
        return 1;
    }

    // Host controls main loop
    while (is_running()) {
        float dt = get_delta_time();

        game_tick(dt);  // Call into HL

        render_frame();  // Host rendering
        swap_buffers();
    }

    game_shutdown();
    return 0;
}
```

---

### Example 2: Worker Thread with HL Callbacks

```c
// worker_thread.c
#include "hlffi.h"
#include <pthread.h>

typedef struct {
    vclosure *callback;
    bool running;
} worker_context;

// Worker thread function
void *worker_thread_main(void *param) {
    worker_context *ctx = (worker_context*)param;

    // 1. CRITICAL: Register this thread with HL
    static void *stack_top = NULL;
    hl_register_thread(&stack_top);

    while (ctx->running) {
        // 2. Perform blocking I/O
        hl_blocking(true);
        char *data = fetch_data_from_network();  // Blocking
        hl_blocking(false);

        // 3. Call HL callback with result
        if (ctx->callback && data) {
            vdynamic *str_arg = hl_alloc_dynamic(&hlt_bytes);
            str_arg->v.bytes = (vbyte*)data;

            hl_dyn_call(ctx->callback, &str_arg, 1);
        }

        sleep(1);  // Poll every second
    }

    // 4. Cleanup before exit
    hl_unregister_thread();
    return NULL;
}

// Called from HL to start worker
static worker_context g_worker = {0};
static pthread_t g_worker_thread;

HL_PRIM void HL_NAME(start_worker)(vclosure *callback) {
    // Store callback with GC root
    if (g_worker.callback) {
        hl_remove_root(&g_worker.callback);
    }
    g_worker.callback = callback;
    if (callback) {
        hl_add_root(&g_worker.callback);
    }

    // Start worker thread
    g_worker.running = true;
    pthread_create(&g_worker_thread, NULL, worker_thread_main, &g_worker);
}

HL_PRIM void HL_NAME(stop_worker)(void) {
    g_worker.running = false;
    pthread_join(g_worker_thread, NULL);

    if (g_worker.callback) {
        hl_remove_root(&g_worker.callback);
        g_worker.callback = NULL;
    }
}

DEFINE_PRIM(_VOID, start_worker, _FUN(_VOID, _BYTES));
DEFINE_PRIM(_VOID, stop_worker, _NO_ARG);
```

**Haxe side**:
```haxe
// Worker.hx
@:hlNative("worker")
class Worker {
    public static function startWorker(callback:(String)->Void):Void {}
    public static function stopWorker():Void {}
}

class Main {
    static function main() {
        Worker.startWorker(function(data:String) {
            trace('Received from worker: $data');
            // This callback executes on worker thread!
            // Be careful with shared state
        });
    }
}
```

---

### Example 3: RAII Guard for hl_blocking (C++)

```cpp
// HLBlockingGuard.h
#pragma once
#include <hl.h>

// RAII wrapper for hl_blocking
class HLBlockingGuard {
public:
    HLBlockingGuard() {
        hl_blocking(true);
    }

    ~HLBlockingGuard() {
        hl_blocking(false);
    }

    // Non-copyable
    HLBlockingGuard(const HLBlockingGuard&) = delete;
    HLBlockingGuard& operator=(const HLBlockingGuard&) = delete;
};

// Usage example
void CallExternalAPI(const std::string& url) {
    // Auto-balances hl_blocking calls
    HLBlockingGuard guard;

    // This may throw or return early, guard still cleans up
    auto response = HttpClient::Get(url);

    ProcessResponse(response);

    // hl_blocking(false) called automatically
}
```

---

## Part 8: Final Verdict & Recommendations

### Does VM block the thread?

**Answer**: ⚠️ **NUANCED**

| Scenario | Blocks Thread? | Details |
|----------|---------------|---------|
| **VM Init** (`hl_module_init`) | ❌ NO | Returns immediately |
| **Entry Point** (`hl_call0(entry)`) | ❌ NO | Returns immediately |
| **Function Calls** (`hl_dyn_call`) | ❌ NO | Returns when function completes |
| **Standalone Heaps App** (`runMainLoop`) | ✅ YES | Blocks in while loop |
| **Embedded Integration** | ❌ NO | Host controls loop |

**Critical Insight**:
- VM core does NOT block
- Standard applications DO block (in `runMainLoop`)
- Embedded integration should NOT let `runMainLoop` execute

---

### Threading models that work

#### ✅ RECOMMENDED: Main Thread Integration

```
Host Thread:
  Init → hl_call0(entry) → [returns]
  Loop → hl_call0(update) → [returns] → render → [repeat]
```

**Pros**: Simple, deterministic, low overhead
**Cons**: HL must be fast (< 16ms for 60fps)
**Use**: 90% of integrations

---

#### ✅ ADVANCED: Dedicated HL Thread

```
Main Thread:    Render Loop
HL Thread:      hl_call0(update) → [loop]
Sync:           Mutex/Atomic for shared data
```

**Pros**: HL can run at different framerate
**Cons**: Complex, synchronization overhead, GC cost
**Use**: When HL needs independent timing

---

#### ✅ HYBRID: Main + Workers

```
Main Thread:    hl_call0(gameplay_update)
Worker 1:       hl_call0(network_callback)
Worker 2:       hl_call0(asset_loader)
```

**Pros**: Best of both worlds
**Cons**: Most complex
**Use**: AAA games with async systems

---

### Thread-safety requirements for FFI calls

**Requirements**:
1. ✅ Thread MUST be registered via `hl_register_thread()`
2. ✅ Stack top pointer must remain valid (use static/NULL)
3. ✅ Use `hl_blocking()` around external blocking operations
4. ✅ Balance `hl_blocking(true)` / `false` calls
5. ✅ Add GC roots for stored closures

**Guarantees**:
- ✅ `hl_dyn_call()` is thread-safe (when thread registered)
- ✅ Allocations are mutex-protected in multi-threaded mode
- ⚠️ GC stops all threads during collection (performance impact)

---

### Recommended approach for engine integration (e.g., Unreal)

**Pattern**: Main Thread Integration (Pattern 1)

**Steps**:
1. **Init** (BeginPlay):
   - `hlffi_create()` - Registers main thread
   - `hlffi_call_entry()` - Initializes game, returns immediately
   - `hlffi_lookup()` - Find update functions

2. **Tick** (Every Frame):
   - `hlffi_call1(update_func, dt)` - Returns immediately
   - Engine continues with physics, rendering

3. **Shutdown** (EndPlay):
   - `hlffi_destroy()` - Only safe at process exit!

**Critical**: Don't let `runMainLoop()` execute - it blocks!

---

### Code examples of proper threading patterns

See Part 7 above for:
- ✅ Simple main thread integration
- ✅ Worker thread with callbacks
- ✅ RAII guard for `hl_blocking`

---

## Part 9: Impact on HLFFI v3.0 Phase 1 API Design

### Updated Phase 1 API

```c
// ============================================================================
// PHASE 1: VM Lifecycle + Threading
// ============================================================================

// 1.1 VM Creation (Main Thread)
hlffi_vm* hlffi_create(void);
// - Calls hl_global_init()
// - Registers main thread with hl_register_thread(NULL)
// - Returns VM handle

// 1.2 Initialization
hlffi_error_code hlffi_init(hlffi_vm* vm, int argc, char** argv);
// - Calls hl_sys_init()
// - Prepares environment

// 1.3 Bytecode Loading
hlffi_error_code hlffi_load_file(hlffi_vm* vm, const char* path);
hlffi_error_code hlffi_load_memory(hlffi_vm* vm, const void* data, size_t size);
// - Loads and JIT-compiles bytecode

// 1.4 Entry Point
hlffi_error_code hlffi_call_entry(hlffi_vm* vm);
// - Calls entry point (Main.main())
// - RETURNS IMMEDIATELY (does not block)
// - Sets up game state, registers loop functions

// 1.5 Cleanup (ONLY at process exit!)
void hlffi_destroy(hlffi_vm* vm);
// - Calls hl_global_free()
// - ⚠️ Can only be called ONCE per process
// - Do NOT attempt to restart VM

// ============================================================================
// THREADING HELPERS
// ============================================================================

// 2.1 Worker Thread Registration
void hlffi_thread_register(void);
void hlffi_thread_unregister(void);
// - Call from worker threads before/after using HL

// 2.2 Blocking Operations
void hlffi_blocking_begin(void);
void hlffi_blocking_end(void);
// - Wrap external blocking I/O calls
// - Prevents GC from waiting

// 2.3 RAII Guard (C++ only)
#ifdef __cplusplus
class HLFFIBlockingGuard {
public:
    HLFFIBlockingGuard();
    ~HLFFIBlockingGuard();
};
#endif

// ============================================================================
// GC ROOT MANAGEMENT
// ============================================================================

// 3.1 Callback Storage
void hlffi_root_add(void** ptr);
void hlffi_root_remove(void** ptr);
// - Add/remove GC roots for stored closures
// - Prevents GC from collecting callbacks

// ============================================================================
// DOCUMENTATION ADDITIONS
// ============================================================================

/*
THREADING MODEL:

1. MAIN THREAD INTEGRATION (Recommended):
   - VM runs on host's main thread
   - Host controls game loop
   - Call hlffi_call_entry() once (returns immediately)
   - Call update functions each frame
   - Simple, deterministic, fast

2. DEDICATED THREAD (Advanced):
   - VM runs on separate thread
   - Requires careful synchronization
   - Higher GC overhead
   - Only use if needed

CRITICAL: Do NOT let runMainLoop() execute!
- Standard Heaps apps block in runMainLoop()
- For embedding, bypass it and call loop functions directly

EXAMPLE (Unreal Engine):
  void BeginPlay() {
      vm = hlffi_create();
      hlffi_init(vm, 0, NULL);
      hlffi_load_file(vm, "game.hl");
      hlffi_call_entry(vm);  // Returns immediately!
      update_func = hlffi_lookup(vm, "Game.update", 1);
  }

  void Tick(float DeltaTime) {
      hlffi_call1(update_func, dt_arg);  // Returns immediately!
  }
*/
```

---

## Part 10: Key Takeaways

### For FFI Designers

1. ✅ **VM does NOT block** - Entry point returns control
2. ⚠️ **Standard apps DO block** - `runMainLoop()` has while loop
3. ✅ **Host should control thread** - Pattern 1 (Main Thread Integration)
4. ❌ **Don't run runMainLoop** - Bypass it for embedding
5. ✅ **Thread registration required** - All threads must register
6. ⚠️ **GC overhead** - Multi-threaded mode is 2.25x slower
7. ✅ **hl_blocking() critical** - Use around external I/O

---

### For Engine Integrators

1. Use **Main Thread Integration** (Pattern 1) by default
2. Call `hlffi_call_entry()` once during init (returns immediately!)
3. Call update functions each frame from host's tick/update
4. Don't let `runMainLoop()` execute - it blocks!
5. Keep HL code fast (< 16ms for 60fps)
6. Use GC roots for stored callbacks
7. Only use dedicated threads if you need independent framerates

---

### For Library Users

1. **Simple case**: Just use Pattern 1 (main thread)
2. **Complex case**: Add worker threads, register each one
3. **Callbacks**: Always add GC roots when storing
4. **External I/O**: Wrap with `hl_blocking()` calls
5. **Debugging**: Single-threaded is much easier!

---

## Part 11: Sources & References

### GitHub Issues
- [Issue #42 - Multithreading Support](https://github.com/HaxeFoundation/hashlink/issues/42) - Threading implementation, GC overhead analysis
- [Issue #188 - GC Locking Error](https://github.com/HaxeFoundation/hashlink/issues/188) - hl_blocking() usage and balancing
- [Issue #207 - VM Restart Issues](https://github.com/HaxeFoundation/hashlink/issues/207) - Embedding challenges
- [Issue #752 - Stack Top Requirement](https://github.com/HaxeFoundation/hashlink/issues/752) - Thread registration patterns
- [Issue #46 - Calling Haxe from C](https://github.com/HaxeFoundation/hashlink/issues/46) - Callback patterns

### Source Code
- [src/main.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/main.c) - Entry point execution flow
- [src/std/thread.c](https://github.com/HaxeFoundation/hashlink/blob/master/src/std/thread.c) - Thread registration implementation
- [libs/sdl/sdl.c](https://github.com/HaxeFoundation/hashlink/blob/master/libs/sdl/sdl.c) - SDL event loop (non-blocking)
- [hxd/App.hx](https://github.com/HeapsIO/heaps/blob/master/hxd/App.hx) - Heaps app structure
- [hxd/System.hl.hx](https://github.com/HeapsIO/heaps/blob/master/hxd/System.hl.hx) - Main loop implementation

### Documentation
- [HashLink GC Blog](https://haxe.org/blog/hashlink-gc/) - GC architecture and thread safety
- [HashLink In Depth - Part 2](https://haxe.org/blog/hashlink-in-depth-p2/) - Type system and threading
- [C API Documentation](https://github.com/HaxeFoundation/hashlink/wiki/C-API-Documentation) - Official API reference
- [Hello HashLink](https://heaps.io/documentation/hello-hashlink.html) - Heaps integration guide
- [Haxe Threading Manual](https://haxe.org/manual/std-threading.html) - Threading API reference

### Community
- [Haxe Community Forum](https://community.haxe.org/)
- [HashLink Discussions](https://github.com/HaxeFoundation/hashlink/discussions)

---

**Report Version**: 1.0
**Date Completed**: 2025-11-22
**Confidence Level**: 🟢 **HIGH** - Based on source code analysis, official docs, and issue history
**Recommendation**: Use **Main Thread Integration (Pattern 1)** for HLFFI v3.0
