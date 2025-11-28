/**
 * Haxe test class for Phase 1 - Threading Tests (Simple/Non-blocking version)
 * For testing without infinite loops
 */
class ThreadingSimple {
    public static var callCount:Int = 0;
    public static var lastValue:Int = 0;
    public static var message:String = "";

    /**
     * Entry point - initialize but don't block
     */
    public static function main():Void {
        trace("ThreadingSimple initialized - non-blocking mode");
        // Don't block - return immediately
    }

    /**
     * Simple increment counter
     */
    public static function incrementCounter():Void {
        callCount++;
        trace('Counter incremented to: ${callCount}');
    }

    /**
     * Set a value
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
     * Set a message
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
        trace('Expensive operation done: ${iterations} iterations, result: ${result}');
        return result;
    }
}
