/**
 * Minimal Enum Test - Concrete types only
 */

/* Simple enum without generics */
enum Color {
    Red;
    Green;
    Blue;
}

/* Simple enum with one Int parameter */
enum Status {
    Active(id:Int);
    Inactive;
}

class MinimalEnumTest {
    /* Create Color.Red */
    public static function createRed():Color {
        return Red;
    }

    /* Create Color.Green */
    public static function createGreen():Color {
        return Green;
    }

    /* Create Status.Active */
    public static function createActive():Status {
        return Active(42);
    }

    /* Create Status.Inactive */
    public static function createInactive():Status {
        return Inactive;
    }

    /* Get color name */
    public static function getColorName(c:Color):Int {
        return switch (c) {
            case Red: 0;
            case Green: 1;
            case Blue: 2;
        }
    }

    public static function main() {
        /* Empty */
    }
}
