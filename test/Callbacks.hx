/**
 * Callback Test Class
 * Tests C→Haxe callbacks via hlffi_register_callback()
 */

class Callbacks {
    /* Callback storage - C code will set these via hlffi_register_callback
     * Using Dynamic type to accept closures with vdynamic signatures */
    public static var onMessage:Dynamic = null;
    public static var onAdd:Dynamic = null;
    public static var onNotify:Dynamic = null;
    public static var onCompute:Dynamic = null;

    /* Test counters */
    static var messageReceived:String = "";
    static var addResult:Int = 0;
    static var notifyCount:Int = 0;
    static var computeResult:Int = 0;

    public static function main() {
        trace("Callbacks test initialized");
    }

    /**
     * Call the onMessage callback with a string
     */
    public static function callMessageCallback(msg:String):Void {
        if (onMessage != null) {
            onMessage(msg);
        } else {
            trace("ERROR: onMessage callback not set");
        }
    }

    /**
     * Call the onAdd callback with two integers
     */
    public static function callAddCallback(a:Int, b:Int):Int {
        if (onAdd != null) {
            return onAdd(a, b);
        } else {
            trace("ERROR: onAdd callback not set");
            return -1;
        }
    }

    /**
     * Call the onNotify callback (no args)
     */
    public static function callNotifyCallback():Void {
        if (onNotify != null) {
            onNotify();
        } else {
            trace("ERROR: onNotify callback not set");
        }
    }

    /**
     * Call the onCompute callback with three integers
     */
    public static function callComputeCallback(a:Int, b:Int, c:Int):Int {
        if (onCompute != null) {
            return onCompute(a, b, c);
        } else {
            trace("ERROR: onCompute callback not set");
            return -1;
        }
    }

    /**
     * Test helper: Store message received by callback
     */
    public static function storeMessage(msg:String):Void {
        messageReceived = msg;
    }

    /**
     * Test helper: Get stored message
     */
    public static function getStoredMessage():String {
        return messageReceived;
    }

    /**
     * Test helper: Store add result
     */
    public static function storeAddResult(result:Int):Void {
        addResult = result;
    }

    /**
     * Test helper: Get add result
     */
    public static function getAddResult():Int {
        return addResult;
    }

    /**
     * Test helper: Increment notify counter
     */
    public static function incrementNotifyCount():Void {
        notifyCount++;
    }

    /**
     * Test helper: Get notify count
     */
    public static function getNotifyCount():Int {
        return notifyCount;
    }

    /**
     * Test helper: Store compute result
     */
    public static function storeComputeResult(result:Int):Void {
        computeResult = result;
    }

    /**
     * Test helper: Get compute result
     */
    public static function getComputeResult():Int {
        return computeResult;
    }

    /**
     * Reset all test state
     */
    public static function reset():Void {
        messageReceived = "";
        addResult = 0;
        notifyCount = 0;
        computeResult = 0;
        onMessage = null;
        onAdd = null;
        onNotify = null;
        onCompute = null;
    }
}
