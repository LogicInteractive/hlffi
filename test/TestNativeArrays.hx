/**
 * Haxe Test: NativeArray and Struct Arrays
 *
 * Demonstrates:
 * - hl.NativeArray<T> for primitives
 * - Array<Struct> with wrapped elements
 * - hl.NativeArray<Struct> for contiguous struct memory
 *
 * Compile:
 *   haxe -hl test_native_arrays.hl -main TestNativeArrays
 *
 * Run from C:
 *   See test_native_arrays.c for C-side access
 */

/* Simple struct for testing */
@:struct
class Vec3 {
    public var x:Single;
    public var y:Single;
    public var z:Single;

    public inline function new(x:Single = 0, y:Single = 0, z:Single = 0) {
        this.x = x;
        this.y = y;  this.z = z;
    }

    public function toString():String {
        return 'Vec3($x, $y, $z)';
    }
}

@:struct
class Particle {
    public var x:Single;
    public var y:Single;
    public var vx:Single;
    public var vy:Single;
    public var life:Single;

    public inline function new() {
        this.x = 0;
        this.y = 0;
        this.vx = 0;
        this.vy = 0;
        this.life = 0;
    }
}

class TestNativeArrays {
    /* Test 1: Regular Array<Int> */
    public static function testRegularArray():Array<Int> {
        var arr = new Array<Int>();
        for (i in 0...10) {
            arr.push(i * 10);
        }
        trace('Regular Array<Int>: ${arr}');
        return arr;
    }

    /* Test 2: hl.NativeArray<Int> */
    public static function testNativeArrayInt():hl.NativeArray<Int> {
        var arr = new hl.NativeArray<Int>(10);
        for (i in 0...10) {
            arr[i] = i * 10;
        }
        trace('NativeArray<Int>: length=${arr.length}');
        trace('  [0]=${arr[0]}, [5]=${arr[5]}, [9]=${arr[9]}');
        return arr;
    }

    /* Test 3: hl.NativeArray<Single> (F32) */
    public static function testNativeArrayF32():hl.NativeArray<Single> {
        var arr = new hl.NativeArray<Single>(5);
        for (i in 0...5) {
            arr[i] = i * 0.5;
        }
        trace('NativeArray<Single>:');
        for (i in 0...5) {
            trace('  [$i]=${arr[i]}');
        }
        return arr;
    }

    /* Test 4: hl.NativeArray<Float> (F64) */
    public static function testNativeArrayF64():hl.NativeArray<Float> {
        var arr = new hl.NativeArray<Float>(3);
        arr[0] = 3.141592653589793;  // π
        arr[1] = 2.718281828459045;  // e
        arr[2] = 1.414213562373095;  // √2
        trace('NativeArray<Float>:');
        trace('  π=${arr[0]}');
        trace('  e=${arr[1]}');
        trace('  √2=${arr[2]}');
        return arr;
    }

    /* Test 5: Array<Vec3> - Wrapped struct elements */
    public static function testArrayStruct():Array<Vec3> {
        var arr = new Array<Vec3>();
        arr.push(new Vec3(1, 0, 0));
        arr.push(new Vec3(0, 1, 0));
        arr.push(new Vec3(0, 0, 1));

        trace('Array<Vec3>: ${arr.length} elements');
        for (i in 0...arr.length) {
            trace('  [$i]=${arr[i]}');
        }
        return arr;
    }

    /* Test 6: hl.NativeArray<Vec3> - Contiguous struct memory */
    public static function testNativeArrayStruct():hl.NativeArray<Vec3> {
        var arr = new hl.NativeArray<Vec3>(100);

        /* Initialize vectors */
        for (i in 0...100) {
            arr[i] = new Vec3(i * 1.0, i * 2.0, i * 3.0);
        }

        trace('NativeArray<Vec3>: ${arr.length} elements (contiguous memory)');
        trace('  [0]=${arr[0]}');
        trace('  [50]=${arr[50]}');
        trace('  [99]=${arr[99]}');

        return arr;
    }

    /* Test 7: Performance - Large particle system */
    public static function testParticleSystem():hl.NativeArray<Particle> {
        final COUNT = 10000;
        var particles = new hl.NativeArray<Particle>(COUNT);

        /* Initialize particles */
        for (i in 0...COUNT) {
            particles[i].x = Math.random() * 800;
            particles[i].y = Math.random() * 600;
            particles[i].vx = (Math.random() - 0.5) * 2;
            particles[i].vy = (Math.random() - 0.5) * 2;
            particles[i].life = 1.0;
        }

        trace('Particle system: ${COUNT} particles initialized');
        trace('  Sample particle[0]: x=${particles[0].x}, y=${particles[0].y}');

        /* Simulate one frame (C code would do this much faster) */
        final dt = 0.016;  // 60 FPS
        for (i in 0...COUNT) {
            particles[i].x += particles[i].vx * dt;
            particles[i].y += particles[i].vy * dt;
            particles[i].life -= dt;
        }

        trace('  After update: x=${particles[0].x}, life=${particles[0].life}');

        return particles;
    }

    /* Test 8: From C - Functions callable from C via HLFFI */
    public static function processIntArray(arr:Array<Int>):Int {
        var sum = 0;
        for (val in arr) {
            sum += val;
        }
        trace('processIntArray: sum=$sum');
        return sum;
    }

    public static function processNativeArray(arr:hl.NativeArray<Int>):Int {
        var sum = 0;
        for (i in 0...arr.length) {
            sum += arr[i];
        }
        trace('processNativeArray: sum=$sum');
        return sum;
    }

    public static function createVec3Array():Array<Vec3> {
        return [
            new Vec3(1, 0, 0),
            new Vec3(0, 1, 0),
            new Vec3(0, 0, 1)
        ];
    }

    public static function createNativeVec3Array():hl.NativeArray<Vec3> {
        var arr = new hl.NativeArray<Vec3>(3);
        arr[0] = new Vec3(1, 0, 0);
        arr[1] = new Vec3(0, 1, 0);
        arr[2] = new Vec3(0, 0, 1);
        return arr;
    }

    /* Main entry point */
    public static function main() {
        trace('===========================================');
        trace('  Haxe NativeArray Tests');
        trace('===========================================');

        trace('\n--- Test 1: Regular Array<Int> ---');
        testRegularArray();

        trace('\n--- Test 2: NativeArray<Int> ---');
        testNativeArrayInt();

        trace('\n--- Test 3: NativeArray<Single> (F32) ---');
        testNativeArrayF32();

        trace('\n--- Test 4: NativeArray<Float> (F64) ---');
        testNativeArrayF64();

        trace('\n--- Test 5: Array<Struct> ---');
        testArrayStruct();

        trace('\n--- Test 6: NativeArray<Struct> ---');
        testNativeArrayStruct();

        trace('\n--- Test 7: Performance - Particle System ---');
        testParticleSystem();

        trace('\n===========================================');
        trace('  All tests complete!');
        trace('===========================================');
    }
}
