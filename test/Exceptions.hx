/**
 * Haxe test class for Phase 6 - Exception Handling
 */
class Exceptions {
    public static var safeValue:Int = 42;

    /**
     * Safe method that always succeeds
     */
    public static function safeMethod():String {
        return "Success!";
    }

    /**
     * Method that always throws an exception
     */
    public static function throwException():Void {
        throw "This is a test exception";
    }

    /**
     * Method that throws with a custom message
     */
    public static function throwCustom(message:String):Void {
        throw message;
    }

    /**
     * Method that conditionally throws
     */
    public static function maybeThrow(shouldThrow:Bool):String {
        if (shouldThrow) {
            throw "Conditional exception thrown";
        }
        return "No exception";
    }

    /**
     * Method with multiple operations that throws
     */
    public static function complexFailure():Int {
        var x = 10;
        var y = 20;
        trace("About to throw...");
        throw "Complex operation failed";
        return x + y;  // Never reached
    }

    /**
     * Method that divides (can throw if divisor is zero conceptually)
     */
    public static function divide(a:Int, b:Int):Int {
        if (b == 0) {
            throw "Division by zero";
        }
        return Std.int(a / b);
    }

    /**
     * Helper function that throws (for nested stack traces)
     */
    static function innerThrow():Void {
        throw "Exception from nested function";
    }

    /**
     * Helper function that calls innerThrow (creates deeper stack)
     */
    static function middleThrow():Void {
        innerThrow();
    }

    /**
     * Public method that calls nested functions that throw
     * Creates a multi-level stack trace
     */
    public static function nestedThrow():Void {
        middleThrow();
    }

    /**
     * Entry point
     */
    public static function main():Void {
        trace("Exceptions test initialized");
    }
}
