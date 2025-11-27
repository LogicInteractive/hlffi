/**
 * Phase 3 Test: Static Fields and Methods
 * Tests static member access from C
 */

class Game {
    public static var score:Int = 0;
    public static var playerName:String = "Player";
    public static var isRunning:Bool = false;
    public static var multiplier:Float = 1.0;

    public static function start():Void {
        trace("Game started!");
        isRunning = true;
        score = 0;
    }

    public static function addPoints(points:Int):Void {
        score += points;
        trace('Added $points points, total: $score');
    }

    public static function setMultiplier(m:Float):Void {
        multiplier = m;
        trace('Multiplier set to $m');
    }

    public static function getScore():Int {
        return score;
    }

    public static function greet(name:String):String {
        return 'Hello, $name!';
    }

    public static function add(a:Int, b:Int):Int {
        return a + b;
    }

    public static function multiply(a:Float, b:Float):Float {
        return a * b;
    }

    public static function reset():Void {
        score = 0;
        isRunning = false;
        multiplier = 1.0;
        playerName = "Player";
        trace("Game reset");
    }
}

class StaticTestMain {
    static function main() {
        trace("=== Static Test Program ===");
        Game.start();
        Game.addPoints(100);
        trace('Final score: ${Game.getScore()}');
        trace(Game.greet("Haxe"));
        trace('2 + 3 = ${Game.add(2, 3)}');
        trace('2.5 * 4.0 = ${Game.multiply(2.5, 4.0)}');
        Game.reset();
        trace("=== Test Complete ===");
    }
}
