/**
 * Phase 3 Test: Main entry point
 */
class StaticMain {
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
