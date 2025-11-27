/**
 * Simple Player class for Phase 4 testing
 * Tests: instance creation, field access, method calls
 */
class Player {
    // Instance fields - primitives
    public var health:Int;
    public var maxHealth:Int;
    public var level:Int;
    public var experience:Float;
    public var isAlive:Bool;

    // Instance fields - objects
    public var name:String;

    /**
     * No-argument constructor (Phase 4.1 - Basic test)
     */
    public function new() {
        this.name = "Unnamed";
        this.health = 100;
        this.maxHealth = 100;
        this.level = 1;
        this.experience = 0.0;
        this.isAlive = true;
        trace('Player created: ${this.name}');
    }

    /**
     * Single-argument constructor (Phase 4.2)
     */
    public static function create(name:String):Player {
        var p = new Player();
        p.name = name;
        trace('Player created: ${name}');
        return p;
    }

    /**
     * Instance method - void return
     */
    public function takeDamage(amount:Int):Void {
        health -= amount;
        if (health <= 0) {
            health = 0;
            isAlive = false;
        }
        trace('${name} took ${amount} damage, health: ${health}');
    }

    /**
     * Instance method - int return
     */
    public function getHealth():Int {
        return health;
    }

    /**
     * Instance method - bool return
     */
    public function checkAlive():Bool {
        return isAlive;
    }

    /**
     * Instance method - string return
     */
    public function getName():String {
        return name;
    }

    /**
     * Instance method - float return
     */
    public function getExperience():Float {
        return experience;
    }

    /**
     * Instance method - multiple args
     */
    public function heal(amount:Int):Void {
        health += amount;
        if (health > maxHealth) {
            health = maxHealth;
        }
        trace('${name} healed ${amount}, health: ${health}');
    }

    /**
     * Instance method - complex logic
     */
    public function gainExp(amount:Float):Bool {
        experience += amount;
        if (experience >= 100.0) {
            levelUp();
            return true;
        }
        return false;
    }

    private function levelUp():Void {
        level++;
        experience = 0.0;
        maxHealth += 10;
        health = maxHealth;
        trace('${name} leveled up to ${level}!');
    }

    /**
     * Main entry point for testing
     */
    public static function main():Void {
        trace("Player class initialized - ready for FFI testing");
    }
}
