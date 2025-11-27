package haxe;

/**
 * Custom EntryPoint for HLFFI embedding
 *
 * This overrides the standard EntryPoint.run() which would block
 * in EventLoop.loop(). Instead, we return immediately and let the
 * C code drive the loop via EventLoop.loopOnce().
 *
 * Compatible with both Haxe 4.x and 5.x
 */
class EntryPoint {
    // Required fields for MainLoop compatibility (Haxe 4.x/5.x)
    public static var threadCount:Int = 1;

    @:keep
    public static function run() {
        // Do NOT call EventLoop.main.loop() - it would block!
        // Instead, the C host will call processEvents() each tick.
        trace("EntryPoint: Non-blocking mode (HLFFI embedding)");
    }

    @:keep
    public static function addThread(f:() -> Void):Void {
        // Stub for MainLoop compatibility - just run directly
        f();
    }

    @:keep
    public static function runInMainThread(f:() -> Void):Void {
        // Just run directly since we're single-threaded
        f();
    }

    /**
     * Called by C code each tick to process Haxe events.
     * This runs all pending MainLoop/Timer callbacks.
     */
    @:keep
    public static function processEvents():Bool {
        // Process the thread's event loop (Haxe 4.x/5.x compatible)
        var events = sys.thread.Thread.current().events;
        if (events != null) {
            events.progress();
        }
        // Return true - caller should check via other means
        return true;
    }
}
