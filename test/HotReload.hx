/**
 * Hot Reload Test
 *
 * Base version - compile this to hot_reload_v1.hl with getValue() returning 100
 * To test hot reload, modify getValue() to return 200 and recompile to hot_reload_v2.hl
 *
 * Compile with:
 *   haxe -main HotReload -hl hot_reload_v1.hl   (with getValue() -> 100)
 *   haxe -main HotReload -hl hot_reload_v2.hl   (with getValue() -> 200)
 */
class HotReload {
    static var version:Int = 2;
    static var counter:Int = 0;

    public static function main() {
        trace("HotReload main() called - version " + version);
    }

    public static function getValue():Int {
        return 200;  // V2 returns 200
    }

    public static function getVersion():Int {
        return version;
    }

    public static function increment():Int {
        counter++;
        return counter;
    }

    public static function getCounter():Int {
        return counter;
    }
}
