/**
 * Test class for Phase 7 caching API
 */
class CacheTest {
    public static var counter:Int = 0;

    public static function main() {
        trace("CacheTest initialized");
    }

    // Simple static method with no args
    public static function increment():Void {
        counter++;
    }

    // Static method with args and return value
    public static function add(a:Int, b:Int):Int {
        return a + b;
    }

    // Static method returning string
    public static function greet(name:String):String {
        return "Hello, " + name + "!";
    }

    // Method with float
    public static function multiply(a:Float, b:Float):Float {
        return a * b;
    }
}
