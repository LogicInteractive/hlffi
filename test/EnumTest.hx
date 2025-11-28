/**
 * Enum Test for Phase 5
 * Tests Haxe enum interop with C
 */

/* Test enum: Option type */
enum Option<T> {
    Some(value:T);
    None;
}

/* Test enum: Result type with multiple params */
enum Result<T, E> {
    Ok(value:T);
    Err(error:E);
}

/* Test enum: Simple enum without params */
enum Color {
    Red;
    Green;
    Blue;
}

class EnumTest {
    /* Create Option.Some */
    public static function createSome():Option<Int> {
        return Some(42);
    }

    /* Create Option.None */
    public static function createNone():Option<Int> {
        return None;
    }

    /* Create Result.Ok */
    public static function createOk():Result<String, Int> {
        return Ok("success");
    }

    /* Create Result.Err */
    public static function createErr():Result<String, Int> {
        return Err(404);
    }

    /* Create Color.Red */
    public static function createRed():Color {
        return Red;
    }

    /* Process an Option */
    public static function processOption(opt:Option<Int>):String {
        return switch (opt) {
            case Some(v): "Some";
            case None: "None";
        }
    }

    /* Process a Result */
    public static function processResult(res:Result<String, Int>):String {
        return switch (res) {
            case Ok(v): "Ok";
            case Err(e): "Err";
        }
    }

    /* Check if an Option is Some */
    public static function isSome(opt:Option<Int>):Bool {
        return switch (opt) {
            case Some(_): true;
            case None: false;
        }
    }

    /* Extract value from Option.Some */
    public static function extractValue(opt:Option<Int>):Int {
        return switch (opt) {
            case Some(v): v;
            case None: -1;
        }
    }

    /* Test all enum operations */
    public static function testEnums():Void {
        /* Empty - just for testing from C */
    }

    public static function main() {
        /* Empty - C test calls static methods directly */
    }
}
