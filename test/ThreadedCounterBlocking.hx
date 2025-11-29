/**
 * Threaded counter test - BLOCKING version
 *
 * main() runs a loop with 1 second sleep, counts to 5, then exits.
 * This tests VM restart when Haxe main() blocks until completion.
 *
 * Compile with:
 *   haxe -main ThreadedCounterBlocking -hl threaded_counter_blocking.hl
 */
class ThreadedCounterBlocking {
    static var counter:Int = 0;

    public static function main() {
        trace("ThreadedCounterBlocking starting (blocking mode)...");

        // Run counting loop - count every 1 second until we hit 5
        while (counter < 5) {
            counter++;
            trace('Counter: ${counter}');
            Sys.sleep(1.0);
        }

        trace("Counter reached 5 - main() exiting");
    }

    /**
     * Get current counter value (can be called from C via sync call if needed)
     */
    public static function getCounter():Int {
        return counter;
    }
}
