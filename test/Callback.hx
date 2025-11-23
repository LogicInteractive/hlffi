/**
 * Simple Haxe test for HLFFI non-blocking calls
 * Demonstrates calling Haxe functions from C
 */
class Callback {
    static function main() {
        trace("Callback test initialized");
    }

    // Simple function that returns an integer
    public static function add(a: Int, b: Int): Int {
        trace('add($a, $b) called from C');
        return a + b;
    }

    // Function that processes a string
    public static function greet(name: String): String {
        trace('greet("$name") called from C');
        return "Hello, " + name + "!";
    }

    // Function with no return value
    public static function printMessage(msg: String): Void {
        trace('printMessage: $msg');
    }

    // Function that does some "work"
    public static function calculate(n: Int): Int {
        trace('calculate($n) called');
        var result = 0;
        for (i in 0...n) {
            result += i;
        }
        trace('calculate result: $result');
        return result;
    }
}
