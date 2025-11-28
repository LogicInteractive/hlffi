/**
 * Example Haxe game class for THREADED mode demonstration
 *
 * Compile with:
 *   haxe -main Game -hl game.hl
 */
class Game {
    static var frameCount:Int = 0;
    static var totalTime:Float = 0.0;

    /**
     * Entry point - called when VM thread starts
     */
    public static function main() {
        trace("Game initialized on VM thread");
        trace("Waiting for commands from host...");
    }

    /**
     * Update game state
     */
    public static function update(delta:Float):Void {
        frameCount++;
        totalTime += delta;
        trace('Game.update() - frame ${frameCount}, delta=${delta}, totalTime=${totalTime}');
    }

    /**
     * Render frame
     */
    public static function render():Void {
        trace('Game.render() - frame ${frameCount}');
    }

    /**
     * Save game (simulated async operation)
     */
    public static function save():Void {
        trace("Game.save() - Saving game state...");
        // Simulate some work
        var sum = 0;
        for (i in 0...100000) {
            sum += i;
        }
        trace("Game.save() - Save complete!");
    }

    /**
     * Get frame count
     */
    public static function getFrameCount():Int {
        return frameCount;
    }

    /**
     * Get total elapsed time
     */
    public static function getTotalTime():Float {
        return totalTime;
    }
}

/**
 * Example Player class
 */
class Player {
    static var x:Float = 100.0;
    static var y:Float = 200.0;
    static var health:Int = 100;
    static var name:String = "Hero";

    public static function getX():Float {
        return x;
    }

    public static function getY():Float {
        return y;
    }

    public static function setPosition(newX:Float, newY:Float):Void {
        x = newX;
        y = newY;
        trace('Player.setPosition(${x}, ${y})');
    }

    public static function getHealth():Int {
        return health;
    }

    public static function setHealth(h:Int):Void {
        health = h;
        trace('Player.setHealth(${health})');
    }

    public static function getName():String {
        return name;
    }

    public static function setName(n:String):Void {
        name = n;
        trace('Player.setName("${name}")');
    }
}
