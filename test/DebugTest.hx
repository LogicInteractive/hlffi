/**
 * Debug Test - Use with VSCode HashLink Debugger
 *
 * Compile with: haxe --debug -hl debug_test.hl -main DebugTest
 *
 * This test provides breakpoint targets and stepping opportunities.
 * Set breakpoints on any line and attach the VSCode debugger.
 */
class DebugTest {
    static var counter:Int = 0;
    static var message:String = "Hello from DebugTest";

    public static function main() {
        trace("=== Debug Test Started ===");
        trace("Set breakpoints and attach VSCode debugger on port 6112");
        trace("");

        // Simple loop - good for stepping
        for (i in 0...5) {
            counter = i;
            var squared = i * i;
            trace('Loop iteration $i: squared = $squared');  // Set breakpoint here
        }

        // Function call - good for step into/over
        var result = calculate(10, 20);
        trace('Calculate result: $result');

        // Object creation - inspect variables
        var player = new Player("TestPlayer", 100);
        player.takeDamage(25);
        trace('Player health: ${player.health}');

        // Nested calls
        var fibonacci = fib(10);
        trace('Fibonacci(10) = $fibonacci');

        trace("");
        trace("=== Debug Test Complete ===");
        trace("If you hit breakpoints, debugging is working!");
    }

    static function calculate(a:Int, b:Int):Int {
        var sum = a + b;      // Breakpoint: inspect a, b, sum
        var product = a * b;  // Breakpoint: inspect product
        return sum + product;
    }

    static function fib(n:Int):Int {
        if (n <= 1) return n;  // Breakpoint: base case
        return fib(n - 1) + fib(n - 2);  // Breakpoint: recursive case
    }
}

class Player {
    public var name:String;
    public var health:Int;
    public var maxHealth:Int;

    public function new(name:String, maxHealth:Int) {
        this.name = name;
        this.maxHealth = maxHealth;
        this.health = maxHealth;
        trace('Player "$name" created with $maxHealth HP');
    }

    public function takeDamage(amount:Int):Void {
        var oldHealth = health;
        health -= amount;
        if (health < 0) health = 0;
        trace('$name took $amount damage: $oldHealth -> $health');  // Breakpoint
    }

    public function heal(amount:Int):Void {
        var oldHealth = health;
        health += amount;
        if (health > maxHealth) health = maxHealth;
        trace('$name healed $amount: $oldHealth -> $health');
    }
}
