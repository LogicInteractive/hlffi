/**
 * Phase 5 Test: Array Operations
 * Tests C↔Haxe array passing and manipulation
 */

class Arrays {
    /* Test data */
    public static var intArray:Array<Int> = [1, 2, 3, 4, 5];
    public static var floatArray:Array<Float> = [1.1, 2.2, 3.3];
    public static var stringArray:Array<String> = ["hello", "world", "test"];
    public static var mixedArray:Array<Dynamic> = [42, "text", 3.14, true];
    public static var emptyArray:Array<Int> = [];

    /* Test methods - return arrays */
    public static function getIntArray():Array<Int> {
        return [10, 20, 30, 40, 50];
    }

    public static function getFloatArray():Array<Float> {
        return [1.5, 2.5, 3.5];
    }

    public static function getStringArray():Array<String> {
        return ["alpha", "beta", "gamma"];
    }

    public static function getDynamicArray():Array<Dynamic> {
        return [100, "mixed", 99.9, false];
    }

    public static function getEmptyArray():Array<Int> {
        return [];
    }

    public static function getSingleElement():Array<Int> {
        return [999];
    }

    /* Test methods - receive arrays from C */
    public static function sumIntArray(arr:Array<Int>):Int {
        var sum = 0;
        for (val in arr) {
            sum += val;
        }
        return sum;
    }

    public static function countStrings(arr:Array<String>):Int {
        return arr.length;
    }

    public static function firstElement(arr:Array<Int>):Int {
        if (arr.length == 0) return -1;
        return arr[0];
    }

    public static function lastElement(arr:Array<Int>):Int {
        if (arr.length == 0) return -1;
        return arr[arr.length - 1];
    }

    /* Test methods - modify arrays */
    public static function doubleValues(arr:Array<Int>):Array<Int> {
        var result = [];
        for (val in arr) {
            result.push(val * 2);
        }
        return result;
    }

    public static function reverseArray(arr:Array<String>):Array<String> {
        var result = [];
        var i = arr.length - 1;
        while (i >= 0) {
            result.push(arr[i]);
            i--;
        }
        return result;
    }

    /* Entry point */
    public static function main() {
        trace("Arrays test initialized");
    }
}
