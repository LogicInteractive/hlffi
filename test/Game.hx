/**
 * Phase 3 Test: Game class with static members
 * Extended for comprehensive testing
 */
class Game {
    // Basic types
    public static var score:Int = 100;
    public static var playerName:String = "TestPlayer";
    public static var isRunning:Bool = true;
    public static var multiplier:Float = 2.5;

    // Edge case values
    public static var negativeInt:Int = -42;
    public static var zeroInt:Int = 0;
    public static var largeInt:Int = 2147483647;  // Max int32
    public static var emptyString:String = "";
    public static var unicodeString:String = "Hello, 世界! 🎮";
    public static var specialChars:String = "Tab:\tNewline:\nQuote:\"";
    public static var zeroFloat:Float = 0.0;
    public static var negativeFloat:Float = -3.14159;
    public static var smallFloat:Float = 0.0000001;
    public static var largeFloat:Float = 1.7976931348623157e+308;
    public static var falseValue:Bool = false;

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

    // === Extended test methods ===

    // Boolean operations
    public static function isActive():Bool {
        return isRunning;
    }

    public static function setActive(active:Bool):Void {
        isRunning = active;
    }

    public static function toggleRunning():Bool {
        isRunning = !isRunning;
        return isRunning;
    }

    public static function andBool(a:Bool, b:Bool):Bool {
        return a && b;
    }

    public static function orBool(a:Bool, b:Bool):Bool {
        return a || b;
    }

    public static function notBool(a:Bool):Bool {
        return !a;
    }

    // Integer edge cases
    public static function negate(n:Int):Int {
        return -n;
    }

    public static function subtract(a:Int, b:Int):Int {
        return a - b;
    }

    public static function divideInt(a:Int, b:Int):Int {
        return Std.int(a / b);
    }

    public static function modulo(a:Int, b:Int):Int {
        return a % b;
    }

    public static function absInt(n:Int):Int {
        return n < 0 ? -n : n;
    }

    public static function maxInt(a:Int, b:Int):Int {
        return a > b ? a : b;
    }

    public static function minInt(a:Int, b:Int):Int {
        return a < b ? a : b;
    }

    // Float operations
    public static function divide(a:Float, b:Float):Float {
        return a / b;
    }

    public static function absFloat(n:Float):Float {
        return Math.abs(n);
    }

    public static function sqrtFloat(n:Float):Float {
        return Math.sqrt(n);
    }

    public static function powFloat(base:Float, exp:Float):Float {
        return Math.pow(base, exp);
    }

    public static function floorFloat(n:Float):Int {
        return Math.floor(n);
    }

    public static function ceilFloat(n:Float):Int {
        return Math.ceil(n);
    }

    public static function roundFloat(n:Float):Int {
        return Math.round(n);
    }

    // String operations
    public static function concat(a:String, b:String):String {
        return a + b;
    }

    public static function stringLength(s:String):Int {
        return s.length;
    }

    public static function toUpper(s:String):String {
        return s.toUpperCase();
    }

    public static function toLower(s:String):String {
        return s.toLowerCase();
    }

    public static function substring(s:String, start:Int, len:Int):String {
        return s.substr(start, len);
    }

    public static function repeatString(s:String, count:Int):String {
        var result = "";
        for (i in 0...count) {
            result += s;
        }
        return result;
    }

    public static function reverseString(s:String):String {
        var chars = s.split("");
        chars.reverse();
        return chars.join("");
    }

    public static function intToString(n:Int):String {
        return Std.string(n);
    }

    public static function floatToString(n:Float):String {
        return Std.string(n);
    }

    public static function stringToInt(s:String):Int {
        var parsed = Std.parseInt(s);
        return parsed != null ? parsed : 0;
    }

    public static function stringToFloat(s:String):Float {
        var parsed = Std.parseFloat(s);
        return Math.isNaN(parsed) ? 0.0 : parsed;
    }

    // Multiple argument functions (3+ args)
    public static function sum3(a:Int, b:Int, c:Int):Int {
        return a + b + c;
    }

    public static function sum4(a:Int, b:Int, c:Int, d:Int):Int {
        return a + b + c + d;
    }

    public static function avg3(a:Float, b:Float, c:Float):Float {
        return (a + b + c) / 3.0;
    }

    public static function formatScore(name:String, score:Int, level:Int):String {
        return '$name: $score pts (Level $level)';
    }

    // Comparison functions
    public static function isGreater(a:Int, b:Int):Bool {
        return a > b;
    }

    public static function isEqual(a:Int, b:Int):Bool {
        return a == b;
    }

    public static function compareStrings(a:String, b:String):Int {
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    // Void return tests
    public static function doNothing():Void {
        // Does nothing
    }

    public static function printMessage(msg:String):Void {
        trace(msg);
    }

    // Return self-referencing values
    public static function getPlayerName():String {
        return playerName;
    }

    public static function getMultiplier():Float {
        return multiplier;
    }

    public static function getIsRunning():Bool {
        return isRunning;
    }

    // === Game Loop / Timer Test Methods ===

    public static var frameCount:Int = 0;
    public static var totalTime:Float = 0.0;
    public static var lastDelta:Float = 0.0;

    public static function update(deltaTime:Float):Void {
        if (!isRunning) return;

        frameCount++;
        totalTime += deltaTime;
        lastDelta = deltaTime;

        // Trace each frame to verify Haxe code is executing
        trace('Frame $frameCount: dt=${Math.round(deltaTime * 1000)}ms, total=${Math.round(totalTime * 1000)}ms');

        // Every 10 frames, add points based on multiplier
        if (frameCount % 10 == 0) {
            var points = Math.round(10 * multiplier);
            score += points;
            trace('  +$points points! Score: $score');
        }
    }

    public static function getFrameCount():Int {
        return frameCount;
    }

    public static function getTotalTime():Float {
        return totalTime;
    }

    public static function getLastDelta():Float {
        return lastDelta;
    }

    public static function getStatus():String {
        return 'Frame: $frameCount, Time: ${Math.round(totalTime * 1000)}ms, Score: $score';
    }

    public static function resetGameLoop():Void {
        frameCount = 0;
        totalTime = 0.0;
        lastDelta = 0.0;
        score = 0;
        isRunning = true;
        multiplier = 1.0;
    }
}
