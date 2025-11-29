/**
 * HLFFI Callback Example: Game Callbacks (Haxe side)
 *
 * Demonstrates how Haxe game logic calls C callbacks for events.
 * This is the Haxe counterpart to callback_event_system.c
 */

class GameCallbacks {
    /* C callbacks - set by C code via hlffi_set_static_field()
     * These are Dynamic because C callbacks expect vdynamic* args */
    public static var onPlayerScored:Dynamic = null;      /* (Int) -> Void */
    public static var onPlayerDamaged:Dynamic = null;     /* (Int, String) -> Void */
    public static var onButtonClicked:Dynamic = null;     /* (String) -> Void */
    public static var onLevelComplete:Dynamic = null;     /* (Int, Int) -> Int */
    public static var getPlayerStats:Dynamic = null;      /* () -> String */

    /**
     * Simulate a game session
     * This calls various C callbacks to demonstrate event flow
     */
    public static function simulateGame():Void {
        trace("\n[Haxe] Starting game simulation...");

        /* Level 1 */
        trace("\n[Haxe] Level 1 starting");

        /* Player collects coins */
        if (onPlayerScored != null) {
            trace("[Haxe] Player collected coin");
            onPlayerScored(100);  /* Calls C function! */

            trace("[Haxe] Player collected gem");
            onPlayerScored(500);  /* Calls C again! */
        }

        /* Player gets hit */
        if (onPlayerDamaged != null) {
            trace("[Haxe] Enemy attacked player");
            onPlayerDamaged(1, "Goblin");  /* Calls C with 2 args! */
        }

        /* Check stats */
        if (getPlayerStats != null) {
            trace("[Haxe] Checking player stats");
            var stats:Dynamic = getPlayerStats();  /* C returns String */
            trace("[Haxe] Current stats: " + stats);
        }

        /* Complete level */
        if (onLevelComplete != null) {
            trace("[Haxe] Level 1 finished");
            var bonus:Dynamic = onLevelComplete(1, 45);  /* Level 1, 45 seconds */
            trace("[Haxe] Bonus received: " + bonus);
        }

        /* Level 2 */
        trace("\n[Haxe] Level 2 starting");

        if (onPlayerScored != null) {
            trace("[Haxe] Player defeated enemy");
            onPlayerScored(1000);
        }

        if (onPlayerDamaged != null) {
            trace("[Haxe] Boss attacked player");
            onPlayerDamaged(2, "Boss");  /* Ouch! */
        }

        if (onLevelComplete != null) {
            trace("[Haxe] Level 2 finished");
            var bonus:Dynamic = onLevelComplete(2, 120);  /* Level 2, slower */
            trace("[Haxe] Bonus received: " + bonus);
        }

        /* UI interaction */
        if (onButtonClicked != null) {
            trace("\n[Haxe] Simulating UI clicks");
            onButtonClicked("pause");
            onButtonClicked("restart");
        }

        /* Final stats */
        if (getPlayerStats != null) {
            trace("\n[Haxe] Final stats:");
            var stats:Dynamic = getPlayerStats();
            trace("[Haxe] " + stats);
        }

        trace("\n[Haxe] Game simulation complete!");
    }

    /**
     * Called by C when game is over
     */
    public static function onGameOver():Void {
        trace("[Haxe] Received game over notification from C");
        trace("[Haxe] Showing game over screen...");
    }

    /**
     * Called by C to restart the game
     */
    public static function restartGame():Void {
        trace("[Haxe] Received restart command from C");
        trace("[Haxe] Resetting game state...");
    }

    /**
     * Entry point
     */
    public static function main():Void {
        trace("[Haxe] GameCallbacks module loaded");
        trace("[Haxe] Waiting for C to set callbacks...");
    }
}
