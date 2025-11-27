/**
 * MainLoop Test: Demonstrates external mainloop control
 *
 * This class uses haxe.MainLoop to schedule callbacks.
 * When embedded, C code drives the loop via EventLoop.loopOnce().
 */
import haxe.MainLoop;

class MainLoopTest {
    // State tracking
    public static var tickCount:Int = 0;
    public static var timerFired:Int = 0;
    public static var lastMessage:String = "";
    public static var isRunning:Bool = false;

    // The registered MainLoop callback
    static var mainLoopHandle:MainLoop.MainLoopEvent = null;

    public static function main() {
        trace("MainLoopTest: Initializing...");
        isRunning = true;

        // Register a MainLoop callback (runs every frame when loopOnce is called)
        mainLoopHandle = MainLoop.add(onTick);

        trace("MainLoopTest: MainLoop callback registered");
        trace("MainLoopTest: Ready for external loopOnce() calls");

        // Entry point returns immediately
        // (normally EntryPoint.run() would block, but we override it)
    }

    // Called every time EventLoop.loopOnce() is invoked
    static function onTick():Void {
        tickCount++;
        lastMessage = 'Tick #$tickCount';

        // Fire "timer" every 10 ticks
        if (tickCount % 10 == 0) {
            timerFired++;
            trace('MainLoopTest: Timer fired ($timerFired times)');
        }
    }

    // API for C to query state
    public static function getTickCount():Int {
        return tickCount;
    }

    public static function getTimerFired():Int {
        return timerFired;
    }

    public static function getLastMessage():String {
        return lastMessage;
    }

    public static function getIsRunning():Bool {
        return isRunning;
    }

    // API for C to control
    public static function stop():Void {
        if (mainLoopHandle != null) {
            mainLoopHandle.stop();
            mainLoopHandle = null;
        }
        isRunning = false;
        trace("MainLoopTest: Stopped");
    }

    public static function reset():Void {
        tickCount = 0;
        timerFired = 0;
        lastMessage = "";
        isRunning = true;

        // Re-register if stopped
        if (mainLoopHandle == null) {
            mainLoopHandle = MainLoop.add(onTick);
        }
        trace("MainLoopTest: Reset");
    }
}
