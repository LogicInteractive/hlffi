/**
 * Threaded counter test for VM restart testing
 *
 * This uses a non-blocking main() - the C host drives the counting via sync calls.
 * Main() just initializes and returns, then the VM thread processes messages.
 *
 * Compile with:
 *   haxe -main ThreadedCounter -hl threaded_counter.hl
 */
class ThreadedCounter {
    static var counter:Int = 0;

    public static function main() {
        trace("ThreadedCounter initialized (non-blocking mode)");
        // Return immediately - C host drives via message calls
    }

    /**
     * Increment counter and sleep 1 second (called from C via sync call)
     * Returns the new counter value
     */
    public static function incrementAndWait():Int {
        counter++;
        trace('Counter: ${counter}');
        Sys.sleep(1.0);  // Sleep in the VM thread
        return counter;
    }

    /**
     * Get current counter value
     */
    public static function getCounter():Int {
        return counter;
    }

    /**
     * Reset counter to 0
     */
    public static function reset():Void {
        counter = 0;
        trace("Counter reset to 0");
    }
}
