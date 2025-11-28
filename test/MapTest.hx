/**
 * Map Test for Phase 5
 */

class MapTest {
    /* IntMap - most common */
    public static function createIntMap():Map<Int, String> {
        var map = new Map<Int, String>();
        map.set(1, "one");
        map.set(2, "two");
        map.set(3, "three");
        Sys.println("[HAXE] Created IntMap with 3 entries");
        return map;
    }

    public static function processIntMap(map:Map<Int, String>):String {
        var result = "";
        for (key in map.keys()) {
            var val = map.get(key);
            Sys.println('[HAXE] map[$key] = $val');
            result += val + " ";
        }
        return result;
    }

    /* StringMap */
    public static function createStringMap():Map<String, Int> {
        var map = new Map<String, Int>();
        map.set("a", 10);
        map.set("b", 20);
        map.set("c", 30);
        Sys.println("[HAXE] Created StringMap with 3 entries");
        return map;
    }

    public static function processStringMap(map:Map<String, Int>):Int {
        var sum = 0;
        for (key in map.keys()) {
            var val = map.get(key);
            Sys.println('[HAXE] map["$key"] = $val');
            sum += val;
        }
        return sum;
    }

    /* Map operations */
    public static function testMapExists(map:Map<Int, String>, key:Int):Bool {
        var exists = map.exists(key);
        Sys.println('[HAXE] map.exists($key) = $exists');
        return exists;
    }

    public static function testMapSize(map:Map<Int, String>):Int {
        var size = Lambda.count(map);
        Sys.println('[HAXE] map size = $size');
        return size;
    }

    public static function main() {
        Sys.println("==========================================");
        Sys.println("  Phase 5: Map Tests - Haxe Side");
        Sys.println("==========================================");

        Sys.println("\n--- IntMap ---");
        var intMap = createIntMap();
        processIntMap(intMap);

        Sys.println("\n--- StringMap ---");
        var strMap = createStringMap();
        processStringMap(strMap);

        Sys.println("\n--- Map Operations ---");
        testMapExists(intMap, 2);
        testMapExists(intMap, 99);
        testMapSize(intMap);

        Sys.println("\n[HAXE] Map tests complete!");
    }
}
