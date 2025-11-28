/**
 * Phase 5 Test: Map Operations
 * Tests C<->Haxe map passing and manipulation
 */

class MapTest {
    public static function createIntMap():Map<Int, String> {
        var map = new Map<Int, String>();
        map.set(1, "one");
        map.set(2, "two");
        map.set(3, "three");
        return map;
    }

    public static function createStringMap():Map<String, Int> {
        var map = new Map<String, Int>();
        map.set("a", 10);
        map.set("b", 20);
        map.set("c", 30);
        return map;
    }

    public static function processIntMap(map:Map<Int, String>):String {
        var result = "Keys: ";
        for (key in map.keys()) {
            result += key + "=" + map.get(key) + " ";
        }
        return result;
    }

    /** Test exists method - needed to ensure it's included in bytecode for C FFI */
    public static function checkIntMapExists(map:Map<Int, String>, key:Int):Bool {
        return map.exists(key);
    }

    /** Test exists method for StringMap */
    public static function checkStringMapExists(map:Map<String, Int>, key:String):Bool {
        return map.exists(key);
    }

    /** Test get for StringMap */
    public static function getStringMapValue(map:Map<String, Int>, key:String):Int {
        return map.get(key);
    }

    public static function main() {
        trace("MapTest initialized");
        // Call exists to ensure it's included
        var intMap = createIntMap();
        trace("IntMap exists(1): " + intMap.exists(1));
        trace("IntMap exists(99): " + intMap.exists(99));

        var strMap = createStringMap();
        trace("StringMap exists('a'): " + strMap.exists("a"));
        trace("StringMap get('b'): " + strMap.get("b"));
    }
}
