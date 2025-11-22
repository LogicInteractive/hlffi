/**
 * Simple Haxe test program for HLFFI
 *
 * To compile:
 *   haxe -hl hello.hl -main Main
 *
 * This creates hello.hl which can be loaded by HLFFI examples.
 */
class Main {
    static function main() {
        trace("Hello from Haxe!");
        trace("HLFFI v3.0 - HashLink FFI Library");
        trace("Testing basic VM functionality...");

        // Test basic operations
        var x = 42;
        var y = 3.14;
        var s = "Hello World";

        trace('Integer: $x');
        trace('Float: $y');
        trace('String: $s');

        // Test array
        var arr = [1, 2, 3, 4, 5];
        trace('Array: $arr');
        trace('Array length: ${arr.length}');

        // Test map
        var map = new Map<String, Int>();
        map.set("one", 1);
        map.set("two", 2);
        map.set("three", 3);
        trace('Map keys: ${[for (k in map.keys()) k]}');
        trace('Map values: ${[for (v in map) v]}');

        trace("Test completed successfully!");
    }
}
