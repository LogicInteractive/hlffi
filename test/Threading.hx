/**
 * Haxe test class for Phase 1 - Threading Tests
 * Simple class with static methods for testing threaded mode
 */
class Threading {
    public static var callCount:Int = 0;
    public static var lastValue:Int = 0;
    public static var message:String = "";

    /**
     * Entry point - initialize (may block in threaded mode)
     */
    public static function main():Void {
        trace("Threading test initialized - entering loop");
        // In threaded mode, this simulates a blocking loop (like Android)
        // The VM thread will stay alive and process messages
        while (true) {
            Sys.sleep(0.001);  // 1ms
            // In real app, this would be game loop or event processing
        }
    }

    /**
     * Simple increment counter (for testing sync calls)
     */
    public static function incrementCounter():Void {
        callCount++;
        trace('Counter incremented to: ${callCount}');
    }

    /**
     * Set a value (for testing with parameters)
     */
    public static function setValue(value:Int):Void {
        lastValue = value;
        trace('Value set to: ${value}');
    }

    /**
     * Get current counter value
     */
    public static function getCounter():Int {
        return callCount;
    }

    /**
     * Set a message (for testing string parameters)
     */
    public static function setMessage(msg:String):Void {
        message = msg;
        trace('Message set to: ${msg}');
    }

    /**
     * Get the message
     */
    public static function getMessage():String {
        return message;
    }

    /**
     * Reset all state
     */
    public static function reset():Void {
        callCount = 0;
        lastValue = 0;
        message = "";
        trace("State reset");
    }

    /**
     * Expensive operation for stress testing
     */
    public static function expensiveOperation(iterations:Int):Int {
        var result = 0;
        for (i in 0...iterations) {
            result += i * i;
        }
        trace('Expensive operation done: ${iterations} iterations');
        return result;
    }
}
