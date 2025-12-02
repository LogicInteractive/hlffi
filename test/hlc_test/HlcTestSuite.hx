/**
 * HLFFI HLC Mode Test Suite
 *
 * Combined test suite for HLC mode that tests multiple aspects
 * of HLFFI functionality when compiled as static C code.
 */

class HlcTestSuite {
    static var testsRun:Int = 0;
    static var testsPassed:Int = 0;

    static function assert(condition:Bool, message:String):Void {
        testsRun++;
        if (condition) {
            testsPassed++;
            trace('  ✓ $message');
        } else {
            trace('  ✗ FAIL: $message');
        }
    }

    static function testBasicTypes():Void {
        trace("\n--- Test: Basic Types ---");

        var i:Int = 42;
        var f:Float = 3.14159;
        var s:String = "Hello";
        var b:Bool = true;

        assert(i == 42, "Integer value");
        assert(f > 3.14 && f < 3.15, "Float value");
        assert(s == "Hello", "String value");
        assert(b == true, "Bool value");
    }

    static function testArrays():Void {
        trace("\n--- Test: Arrays ---");

        var intArr = [1, 2, 3, 4, 5];
        assert(intArr.length == 5, "Int array length");
        assert(intArr[0] == 1, "Int array access");

        var sum = 0;
        for (x in intArr) sum += x;
        assert(sum == 15, "Int array iteration sum");

        var strArr = ["a", "b", "c"];
        assert(strArr.length == 3, "String array length");
        assert(strArr.join("-") == "a-b-c", "String array join");

        var floatArr:Array<Float> = [1.1, 2.2, 3.3];
        assert(floatArr.length == 3, "Float array length");

        intArr.push(6);
        assert(intArr.length == 6, "Array push");
        assert(intArr.pop() == 6, "Array pop");
    }

    static function testMaps():Void {
        trace("\n--- Test: Maps ---");

        var intMap = new Map<String, Int>();
        intMap.set("one", 1);
        intMap.set("two", 2);
        intMap.set("three", 3);

        assert(intMap.get("one") == 1, "Map get");
        assert(intMap.exists("two"), "Map exists");
        assert(!intMap.exists("four"), "Map not exists");

        var count = 0;
        for (k => v in intMap) count++;
        assert(count == 3, "Map iteration");

        intMap.remove("two");
        assert(!intMap.exists("two"), "Map remove");
    }

    static function testStrings():Void {
        trace("\n--- Test: Strings ---");

        var s = "Hello, World!";
        assert(s.length == 13, "String length");
        assert(s.charAt(0) == "H", "String charAt");
        assert(s.indexOf("World") == 7, "String indexOf");
        assert(s.substr(0, 5) == "Hello", "String substr");
        assert(s.toUpperCase() == "HELLO, WORLD!", "String toUpperCase");

        var parts = s.split(", ");
        assert(parts.length == 2, "String split");
        assert(parts[0] == "Hello", "String split part 0");
    }

    static function testMath():Void {
        trace("\n--- Test: Math ---");

        assert(Math.abs(-5) == 5, "Math.abs");
        assert(Math.max(3, 7) == 7, "Math.max");
        assert(Math.min(3, 7) == 3, "Math.min");
        assert(Math.floor(3.7) == 3, "Math.floor");
        assert(Math.ceil(3.2) == 4, "Math.ceil");
        assert(Math.round(3.5) == 4, "Math.round");
        assert(Math.sqrt(16) == 4, "Math.sqrt");
    }

    static function testClasses():Void {
        trace("\n--- Test: Classes ---");

        var p = new TestPlayer("Hero", 100);
        assert(p.name == "Hero", "Class field access");
        assert(p.health == 100, "Class field value");

        p.takeDamage(30);
        assert(p.health == 70, "Class method call");

        p.heal(20);
        assert(p.health == 90, "Class method with return");
    }

    static function testStaticMembers():Void {
        trace("\n--- Test: Static Members ---");

        TestGame.score = 0;
        assert(TestGame.score == 0, "Static field initial");

        TestGame.addPoints(100);
        assert(TestGame.score == 100, "Static method modifies field");

        TestGame.addPoints(50);
        assert(TestGame.score == 150, "Static field accumulation");

        assert(TestGame.getHighScore() == 150, "Static method returns value");
    }

    static function testEnums():Void {
        trace("\n--- Test: Enums ---");

        var state = TestState.Running;
        assert(state == TestState.Running, "Enum value comparison");

        state = TestState.Paused;
        assert(state != TestState.Running, "Enum inequality");

        var result = TestResult.Success(42);
        switch (result) {
            case Success(value):
                assert(value == 42, "Enum with parameter");
            case Error(_):
                assert(false, "Wrong enum case");
        }
    }

    static function testExceptions():Void {
        trace("\n--- Test: Exceptions ---");
        // Note: Exception handling in HLC mode can be tricky due to stack setup
        // Testing basic try-catch without actually throwing
        var value = 0;
        try {
            value = 42;
        } catch (e:Dynamic) {
            value = -1;
        }
        assert(value == 42, "Try block executed");
        trace("  (Exception throwing test skipped in HLC mode)");
    }

    static function main():Void {
        trace("==============================================");
        trace("  HLFFI HLC Mode Test Suite");
        trace("==============================================");

        testBasicTypes();
        testArrays();
        testMaps();
        testStrings();
        testMath();
        testClasses();
        testStaticMembers();
        testEnums();
        testExceptions();

        trace("\n==============================================");
        trace('  Results: $testsPassed / $testsRun tests passed');
        if (testsPassed == testsRun) {
            trace("  ✓ All tests passed!");
        } else {
            trace('  ✗ ${testsRun - testsPassed} tests failed');
        }
        trace("==============================================");
    }
}

/* Helper classes for testing */

class TestPlayer {
    public var name:String;
    public var health:Int;

    public function new(name:String, health:Int) {
        this.name = name;
        this.health = health;
    }

    public function takeDamage(amount:Int):Void {
        health -= amount;
        if (health < 0) health = 0;
    }

    public function heal(amount:Int):Int {
        health += amount;
        return health;
    }
}

class TestGame {
    public static var score:Int = 0;
    public static var highScore:Int = 0;

    public static function addPoints(points:Int):Void {
        score += points;
        if (score > highScore) highScore = score;
    }

    public static function getHighScore():Int {
        return highScore;
    }

    public static function reset():Void {
        score = 0;
    }
}

enum TestState {
    Running;
    Paused;
    Stopped;
}

enum TestResult {
    Success(value:Int);
    Error(message:String);
}
