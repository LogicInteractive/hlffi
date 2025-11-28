/**
 * Bytes Test for Phase 5
 * Tests hl.Bytes and haxe.io.Bytes interop with C
 */

class BytesTest {
    /* Create raw hl.Bytes */
    public static function createHLBytes():hl.Bytes {
        var bytes = new hl.Bytes(10);
        bytes[0] = 0x48;  // 'H'
        bytes[1] = 0x65;  // 'e'
        bytes[2] = 0x6C;  // 'l'
        bytes[3] = 0x6C;  // 'l'
        bytes[4] = 0x6F;  // 'o'
        bytes[5] = 0;     // null terminator
        Sys.println("[HAXE] Created hl.Bytes with 'Hello'");
        return bytes;
    }

    /* Create haxe.io.Bytes */
    public static function createIOBytes():haxe.io.Bytes {
        var bytes = haxe.io.Bytes.ofString("Haxe Bytes!");
        Sys.println("[HAXE] Created haxe.io.Bytes: length=" + bytes.length);
        return bytes;
    }

    /* Process bytes from C */
    public static function processBytes(bytes:hl.Bytes):String {
        var result = "";
        for (i in 0...10) {
            if (bytes[i] == 0) break;
            result += String.fromCharCode(bytes[i]);
        }
        Sys.println("[HAXE] Processed bytes: '" + result + "'");
        return result;
    }

    /* Read and display bytes */
    public static function displayBytes(bytes:haxe.io.Bytes):Void {
        Sys.println("[HAXE] Bytes length: " + bytes.length);
        var hex = "";
        var limit = bytes.length < 20 ? bytes.length : 20;
        for (i in 0...limit) {
            hex += StringTools.hex(bytes.get(i), 2) + " ";
        }
        Sys.println("[HAXE] First bytes (hex): " + hex);
        Sys.println("[HAXE] As string: " + bytes.toString());
    }

    /* Test blit operation */
    public static function testBlit():Void {
        var src = haxe.io.Bytes.ofString("ABCDEFGH");
        var dst = haxe.io.Bytes.alloc(10);

        dst.blit(0, src, 2, 4);  // Copy "CDEF" to dst[0...]

        Sys.println("[HAXE] Blit test:");
        Sys.println("[HAXE]   src = " + src.toString());
        Sys.println("[HAXE]   dst = " + dst.toString());
    }

    /* Test fill operation */
    public static function testFill():Void {
        var bytes = haxe.io.Bytes.alloc(10);
        bytes.fill(0, 10, 0xFF);

        Sys.println("[HAXE] Fill test:");
        var hex = "";
        for (i in 0...bytes.length) {
            hex += StringTools.hex(bytes.get(i), 2) + " ";
        }
        Sys.println("[HAXE]   bytes (hex): " + hex);
    }

    /* Test compare operation */
    public static function testCompare():Void {
        var a = haxe.io.Bytes.ofString("ABC");
        var b = haxe.io.Bytes.ofString("ABD");

        var cmp = a.compare(b);
        Sys.println("[HAXE] Compare test:");
        Sys.println("[HAXE]   'ABC' vs 'ABD' = " + cmp + " (expected <0)");
    }

    public static function main() {
        Sys.println("==========================================");
        Sys.println("  Phase 5: Bytes Tests - Haxe Side");
        Sys.println("==========================================");

        Sys.println("\n--- Create Bytes ---");
        var hlBytes = createHLBytes();
        var ioBytes = createIOBytes();

        Sys.println("\n--- Display Bytes ---");
        displayBytes(ioBytes);

        Sys.println("\n--- Blit Test ---");
        testBlit();

        Sys.println("\n--- Fill Test ---");
        testFill();

        Sys.println("\n--- Compare Test ---");
        testCompare();

        Sys.println("\n[HAXE] Bytes tests complete!");
    }
}
