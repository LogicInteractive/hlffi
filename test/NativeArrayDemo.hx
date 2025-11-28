/**
 * Demo: NativeArray with Haxe and C output
 */

class NativeArrayDemo {
    /* Regular Array<Int> */
    public static function createIntArray():Array<Int> {
        var arr = [10, 20, 30, 40, 50];
        Sys.println("[HAXE] Created Array<Int>: " + arr);
        return arr;
    }

    public static function processIntArray(arr:Array<Int>):Int {
        var sum = 0;
        for (x in arr) sum += x;
        Sys.println("[HAXE] Processing Array<Int>, sum = " + sum);
        return sum;
    }

    /* NativeArray<Int> */
    public static function createNativeIntArray():hl.NativeArray<Int> {
        var arr = new hl.NativeArray<Int>(5);
        for (i in 0...5) {
            arr[i] = (i + 1) * 100;
        }
        Sys.println("[HAXE] Created NativeArray<Int>[5]");
        Sys.println("[HAXE]   Values: [" + arr[0] + ", " + arr[1] + ", " + arr[2] + ", " + arr[3] + ", " + arr[4] + "]");
        return arr;
    }

    public static function processNativeIntArray(arr:hl.NativeArray<Int>):Int {
        var sum = 0;
        for (i in 0...arr.length) {
            sum += arr[i];
        }
        Sys.println("[HAXE] Processing NativeArray<Int>, sum = " + sum);
        return sum;
    }

    /* NativeArray<Float> */
    public static function createNativeFloatArray():hl.NativeArray<Float> {
        var arr = new hl.NativeArray<Float>(3);
        arr[0] = 3.14159;
        arr[1] = 2.71828;
        arr[2] = 1.41421;
        Sys.println("[HAXE] Created NativeArray<Float>[3]");
        Sys.println("[HAXE]   π = " + arr[0]);
        Sys.println("[HAXE]   e = " + arr[1]);
        Sys.println("[HAXE]   √2 = " + arr[2]);
        return arr;
    }

    public static function processNativeFloatArray(arr:hl.NativeArray<Float>):Float {
        var sum = 0.0;
        for (i in 0...arr.length) {
            sum += arr[i];
        }
        Sys.println("[HAXE] Processing NativeArray<Float>, sum = " + sum);
        return sum;
    }

    /* NativeArray<Single> (F32) */
    public static function createNativeSingleArray():hl.NativeArray<Single> {
        var arr = new hl.NativeArray<Single>(4);
        arr[0] = 1.5;
        arr[1] = 2.5;
        arr[2] = 3.5;
        arr[3] = 4.5;
        Sys.println("[HAXE] Created NativeArray<Single>[4]");
        Sys.println("[HAXE]   Values: [" + arr[0] + ", " + arr[1] + ", " + arr[2] + ", " + arr[3] + "]");
        return arr;
    }

    public static function main() {
        Sys.println("==========================================");
        Sys.println("  NativeArray Demo - Haxe Side");
        Sys.println("==========================================");

        Sys.println("\n--- Creating arrays from Haxe ---");
        var arr1 = createIntArray();
        var arr2 = createNativeIntArray();
        var arr3 = createNativeFloatArray();
        var arr4 = createNativeSingleArray();

        Sys.println("\n[HAXE] Arrays created successfully!");
        Sys.println("[HAXE] Waiting for C to access them...");
    }
}
