/**
 * Complete Array Test Suite for Phase 5
 * Tests all array types: primitives, strings, structs, native arrays
 */

@:struct
class Vec3 {
    public var x:Single;
    public var y:Single;
    public var z:Single;

    public inline function new(x:Single = 0, y:Single = 0, z:Single = 0) {
        this.x = x;
        this.y = y;
        this.z = z;
    }
}

class ArrayTest {
    /* Test 1: Array<Int> */
    public static function getIntArray():Array<Int> {
        return [10, 20, 30, 40, 50];
    }

    public static function processIntArray(arr:Array<Int>):Int {
        var sum = 0;
        for (x in arr) sum += x;
        return sum;
    }

    /* Test 2: Array<Single> (F32) */
    public static function getSingleArray():Array<Single> {
        var arr:Array<Single> = [];
        for (i in 0...5) {
            arr.push(i * 0.5);
        }
        return arr;
    }

    public static function processSingleArray(arr:Array<Single>):Single {
        var sum:Single = 0;
        for (x in arr) sum += x;
        return sum;
    }

    /* Test 3: Array<Float> (F64) */
    public static function getFloatArray():Array<Float> {
        return [3.14, 2.71, 1.41];
    }

    public static function processFloatArray(arr:Array<Float>):Float {
        var sum = 0.0;
        for (x in arr) sum += x;
        return sum;
    }

    /* Test 4: Array<Bool> */
    public static function getBoolArray():Array<Bool> {
        return [true, false, true, true, false];
    }

    public static function processBoolArray(arr:Array<Bool>):Int {
        var count = 0;
        for (b in arr) if (b) count++;
        return count;
    }

    /* Test 5: Array<String> */
    public static function getStringArray():Array<String> {
        return ["Hello", "World", "Haxe", "HashLink"];
    }

    public static function processStringArray(arr:Array<String>):String {
        return arr.join(" ");
    }

    /* Test 6: hl.NativeArray<Int> */
    public static function getNativeIntArray():hl.NativeArray<Int> {
        var arr = new hl.NativeArray<Int>(10);
        for (i in 0...10) {
            arr[i] = i * 100;
        }
        return arr;
    }

    public static function processNativeIntArray(arr:hl.NativeArray<Int>):Int {
        var sum = 0;
        for (i in 0...arr.length) {
            sum += arr[i];
        }
        return sum;
    }

    /* Test 7: hl.NativeArray<Single> (F32) */
    public static function getNativeSingleArray():hl.NativeArray<Single> {
        var arr = new hl.NativeArray<Single>(5);
        for (i in 0...5) {
            arr[i] = i * 1.5;
        }
        return arr;
    }

    public static function processNativeSingleArray(arr:hl.NativeArray<Single>):Single {
        var sum:Single = 0;
        for (i in 0...arr.length) {
            sum += arr[i];
        }
        return sum;
    }

    /* Test 8: hl.NativeArray<Float> (F64) */
    public static function getNativeFloatArray():hl.NativeArray<Float> {
        var arr = new hl.NativeArray<Float>(3);
        arr[0] = 3.141592653589793;
        arr[1] = 2.718281828459045;
        arr[2] = 1.414213562373095;
        return arr;
    }

    public static function processNativeFloatArray(arr:hl.NativeArray<Float>):Float {
        var sum = 0.0;
        for (i in 0...arr.length) {
            sum += arr[i];
        }
        return sum;
    }

    /* Test 9: hl.NativeArray<Bool> */
    public static function getNativeBoolArray():hl.NativeArray<Bool> {
        var arr = new hl.NativeArray<Bool>(5);
        arr[0] = true;
        arr[1] = false;
        arr[2] = true;
        arr[3] = true;
        arr[4] = false;
        return arr;
    }

    public static function processNativeBoolArray(arr:hl.NativeArray<Bool>):Int {
        var count = 0;
        for (i in 0...arr.length) {
            if (arr[i]) count++;
        }
        return count;
    }

    /* Test 10: Array<Vec3> - Wrapped structs */
    public static function getVec3Array():Array<Vec3> {
        var arr:Array<Vec3> = [];
        arr.push(new Vec3(1, 0, 0));
        arr.push(new Vec3(0, 1, 0));
        arr.push(new Vec3(0, 0, 1));
        return arr;
    }

    public static function processVec3Array(arr:Array<Vec3>):Single {
        var sum:Single = 0;
        for (v in arr) {
            sum += v.x + v.y + v.z;
        }
        return sum;
    }

    /* Test 11: hl.NativeArray<Vec3> - Contiguous structs */
    public static function getNativeVec3Array():hl.NativeArray<Vec3> {
        var arr = new hl.NativeArray<Vec3>(100);
        for (i in 0...100) {
            arr[i] = new Vec3(i * 1.0, i * 2.0, i * 3.0);
        }
        return arr;
    }

    public static function processNativeVec3Array(arr:hl.NativeArray<Vec3>):Single {
        var sum:Single = 0;
        for (i in 0...arr.length) {
            sum += arr[i].x + arr[i].y + arr[i].z;
        }
        return sum;
    }

    /* Main entry point */
    public static function main() {
        Sys.println("===========================================");
        Sys.println("  Phase 5 Array Tests - All Types");
        Sys.println("===========================================");

        Sys.println("\n--- Array<Int> ---");
        var arr1 = getIntArray();
        Sys.println('  Created: ${arr1}');
        Sys.println('  Sum: ${processIntArray(arr1)}');

        Sys.println("\n--- Array<Single> ---");
        var arr2 = getSingleArray();
        Sys.println('  Created: ${arr2}');
        Sys.println('  Sum: ${processSingleArray(arr2)}');

        Sys.println("\n--- Array<Float> ---");
        var arr3 = getFloatArray();
        Sys.println('  Created: ${arr3}');
        Sys.println('  Sum: ${processFloatArray(arr3)}');

        Sys.println("\n--- Array<Bool> ---");
        var arr4 = getBoolArray();
        Sys.println('  Created: ${arr4}');
        Sys.println('  True count: ${processBoolArray(arr4)}');

        Sys.println("\n--- Array<String> ---");
        var arr5 = getStringArray();
        Sys.println('  Created: ${arr5}');
        Sys.println('  Joined: ${processStringArray(arr5)}');

        Sys.println("\n--- NativeArray<Int> ---");
        var arr6 = getNativeIntArray();
        Sys.println('  Length: ${arr6.length}');
        Sys.println('  Sum: ${processNativeIntArray(arr6)}');

        Sys.println("\n--- NativeArray<Single> ---");
        var arr7 = getNativeSingleArray();
        Sys.println('  Length: ${arr7.length}');
        Sys.println('  Sum: ${processNativeSingleArray(arr7)}');

        Sys.println("\n--- NativeArray<Float> ---");
        var arr8 = getNativeFloatArray();
        Sys.println('  Length: ${arr8.length}');
        Sys.println('  Sum: ${processNativeFloatArray(arr8)}');

        Sys.println("\n--- NativeArray<Bool> ---");
        var arr9 = getNativeBoolArray();
        Sys.println('  Length: ${arr9.length}');
        Sys.println('  True count: ${processNativeBoolArray(arr9)}');

        Sys.println("\n--- Array<Vec3> ---");
        var arr10 = getVec3Array();
        Sys.println('  Length: ${arr10.length}');
        Sys.println('  Sum: ${processVec3Array(arr10)}');

        Sys.println("\n--- NativeArray<Vec3> ---");
        var arr11 = getNativeVec3Array();
        Sys.println('  Length: ${arr11.length}');
        Sys.println('  Sum: ${processNativeVec3Array(arr11)}');

        Sys.println("\n===========================================");
        Sys.println("  ✓ All Phase 5 array types tested!");
        Sys.println("===========================================");
    }
}
