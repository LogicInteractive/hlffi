/**
 * Haxe test class for Phase 1 Extensions - Event Loop Integration
 * Tests timers, MainLoop callbacks, and async operations
 */
class Timers {
    public static var timerFired:Int = 0;
    public static var delayFired:Int = 0;
    public static var mainLoopFired:Int = 0;
    public static var intervalCount:Int = 0;

    private static var intervalTimer:haxe.Timer = null;

    /**
     * Entry point - initialize but don't block
     */
    public static function main():Void {
        trace("Timers test initialized");
    }

    /**
     * Process the event loop - call this from C to process timers
     * This is what hlffi_update() should call to make timers work
     */
    public static function processEventLoop():Void {
        var loop = sys.thread.Thread.current().events;
        if (loop != null) {
            loop.progress();
        }
    }

    /**
     * Start a one-shot timer (fires once after delay)
     * @param delayMs Delay in milliseconds
     */
    public static function startOneShotTimer(delayMs:Int):Void {
        trace('Starting one-shot timer: ${delayMs}ms');
        haxe.Timer.delay(() -> {
            trace('One-shot timer fired after ${delayMs}ms');
            timerFired++;
        }, delayMs);
    }

    /**
     * Start a delayed callback
     * @param delayMs Delay in milliseconds
     */
    public static function startDelayedCallback(delayMs:Int):Void {
        trace('Starting delayed callback: ${delayMs}ms');
        haxe.Timer.delay(() -> {
            trace('Delayed callback fired after ${delayMs}ms');
            delayFired++;
        }, delayMs);
    }

    /**
     * Add a MainLoop callback (fires on next event loop iteration)
     * Note: MainLoop.add callbacks fire every iteration until removed
     */
    public static function addMainLoopCallback():Void {
        trace("Adding MainLoop callback");
        var fired = false;
        haxe.MainLoop.add(() -> {
            if (!fired) {
                trace("MainLoop callback fired");
                mainLoopFired++;
                fired = true;
            }
        });
    }

    /**
     * Start an interval timer (fires repeatedly)
     * @param intervalMs Interval in milliseconds
     */
    public static function startIntervalTimer(intervalMs:Int):Void {
        trace('Starting interval timer: ${intervalMs}ms');
        if (intervalTimer != null) {
            intervalTimer.stop();
        }
        intervalTimer = new haxe.Timer(intervalMs);
        intervalTimer.run = () -> {
            trace('Interval timer tick ${intervalCount}');
            intervalCount++;
        };
    }

    /**
     * Stop the interval timer
     */
    public static function stopIntervalTimer():Void {
        if (intervalTimer != null) {
            trace("Stopping interval timer");
            intervalTimer.stop();
            intervalTimer = null;
        }
    }

    /**
     * Test multiple timers with different delays
     */
    public static function testMultipleTimers():Void {
        trace("Testing multiple timers");

        startOneShotTimer(10);
        startOneShotTimer(50);
        startOneShotTimer(100);

        addMainLoopCallback();
    }

    /**
     * Get total number of timers that have fired
     */
    public static function getTotalFired():Int {
        return timerFired + delayFired + mainLoopFired;
    }

    /**
     * Reset all counters
     */
    public static function resetCounters():Void {
        timerFired = 0;
        delayFired = 0;
        mainLoopFired = 0;
        intervalCount = 0;
        trace("Counters reset");
    }

    /**
     * Get timer fired count
     */
    public static function getTimerFired():Int {
        return timerFired;
    }

    /**
     * Get delay fired count
     */
    public static function getDelayFired():Int {
        return delayFired;
    }

    /**
     * Get MainLoop fired count
     */
    public static function getMainLoopFired():Int {
        return mainLoopFired;
    }

    /**
     * Get interval count
     */
    public static function getIntervalCount():Int {
        return intervalCount;
    }

    /**
     * Test high-frequency timer (1ms)
     */
    public static function testHighFrequencyTimer():Void {
        trace("Starting high-frequency 1ms timer");
        startOneShotTimer(1);
    }

    /**
     * Test timer precision by setting up multiple timers at different intervals
     */
    public static function testTimerPrecision():Void {
        trace("Testing timer precision");

        // 1ms, 2ms, 5ms, 10ms, 20ms, 50ms, 100ms
        for (delay in [1, 2, 5, 10, 20, 50, 100]) {
            haxe.Timer.delay(() -> {
                trace('Timer ${delay}ms fired');
                timerFired++;
            }, delay);
        }
    }
}
