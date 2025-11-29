# HLFFI User Guide - Part 2: Calling Haxe from C

**How to call Haxe functions and access Haxe data**

[← Getting Started](GUIDE_01_GETTING_STARTED.md) | [Next: Data Exchange →](GUIDE_03_DATA_EXCHANGE.md)

---

## Overview

After loading your Haxe code, you can:
- Call **static methods** on Haxe classes
- Get and set **static variables**
- Create Haxe **objects** and call their methods
- Access **instance fields**

---

## Calling Static Methods

The simplest way to call Haxe code is through static methods.

### Haxe Side

```haxe
class Game
{
    public static function start():Void
    {
        trace("Game started!");
    }

    public static function add(a:Int, b:Int):Int
    {
        return a + b;
    }

    public static function greet(name:String):String
    {
        return "Hello, " + name + "!";
    }
}
```

### C Side - No Arguments, No Return

```c
// Call Game.start()
hlffi_call_static(vm, "Game", "start", 0, NULL);
```

### C Side - With Arguments

```c
// Call Game.add(10, 20)
hlffi_value* a = hlffi_value_int(vm, 10);
hlffi_value* b = hlffi_value_int(vm, 20);
hlffi_value* args[] = {a, b};

hlffi_value* result = hlffi_call_static(vm, "Game", "add", 2, args);

int sum = hlffi_value_as_int(result, 0);  // 30
printf("Sum: %d\n", sum);

// Always free values when done
hlffi_value_free(a);
hlffi_value_free(b);
hlffi_value_free(result);
```

### C Side - With String Argument

```c
// Call Game.greet("World")
hlffi_value* name = hlffi_value_string(vm, "World");
hlffi_value* args[] = {name};

hlffi_value* result = hlffi_call_static(vm, "Game", "greet", 1, args);

char* greeting = hlffi_value_as_string(result);  // "Hello, World!"
printf("%s\n", greeting);

// Free everything (strings need free(), not hlffi_value_free)
free(greeting);
hlffi_value_free(name);
hlffi_value_free(result);
```

---

## Static Variables

You can read and write static variables on Haxe classes.

### Haxe Side

```haxe
class Config
{
    public static var playerName:String = "Player";
    public static var maxHealth:Int = 100;
    public static var difficulty:Float = 1.0;
}
```

### C Side - Reading

```c
// Get Config.maxHealth
hlffi_value* val = hlffi_get_static_field(vm, "Config", "maxHealth");
int health = hlffi_value_as_int(val, 0);
printf("Max health: %d\n", health);  // 100
hlffi_value_free(val);

// Get Config.playerName
hlffi_value* name_val = hlffi_get_static_field(vm, "Config", "playerName");
char* name = hlffi_value_as_string(name_val);
printf("Player: %s\n", name);  // "Player"
free(name);
hlffi_value_free(name_val);
```

### C Side - Writing

```c
// Set Config.maxHealth = 200
hlffi_value* new_health = hlffi_value_int(vm, 200);
hlffi_set_static_field(vm, "Config", "maxHealth", new_health);
hlffi_value_free(new_health);

// Set Config.playerName = "Hero"
hlffi_value* new_name = hlffi_value_string(vm, "Hero");
hlffi_set_static_field(vm, "Config", "playerName", new_name);
hlffi_value_free(new_name);
```

---

## Creating Objects

You can create instances of Haxe classes.

### Haxe Side

```haxe
class Player
{
    public var name:String;
    public var health:Int;

    public function new(playerName:String)
    {
        this.name = playerName;
        this.health = 100;
    }

    public function takeDamage(amount:Int):Void
    {
        health -= amount;
        if (health < 0) health = 0;
        trace('$name took $amount damage, health: $health');
    }

    public function getHealth():Int
    {
        return health;
    }
}
```

### C Side - Creating an Object

```c
// Create new Player("Hero")
hlffi_value* name_arg = hlffi_value_string(vm, "Hero");
hlffi_value* player = hlffi_new(vm, "Player", 1, &name_arg);
hlffi_value_free(name_arg);

if (!player)
{
    printf("Failed to create Player: %s\n", hlffi_get_error(vm));
    return;
}

// Use the player...

// Free when done
hlffi_value_free(player);
```

---

## Accessing Instance Fields

Once you have an object, you can read and write its fields.

### C Side - Reading Fields

```c
// Get player.health
hlffi_value* hp_val = hlffi_get_field(player, "health");
int health = hlffi_value_as_int(hp_val, 0);
printf("Health: %d\n", health);
hlffi_value_free(hp_val);

// Get player.name
hlffi_value* name_val = hlffi_get_field(player, "name");
char* name = hlffi_value_as_string(name_val);
printf("Name: %s\n", name);
free(name);
hlffi_value_free(name_val);
```

### C Side - Writing Fields

```c
// Set player.health = 50
hlffi_value* new_hp = hlffi_value_int(vm, 50);
hlffi_set_field(player, "health", new_hp);
hlffi_value_free(new_hp);
```

### Convenience Functions (Recommended)

The convenience API is much cleaner - use it when possible:

```c
// Reading (one line!)
int health = hlffi_get_field_int(player, "health", 0);
float speed = hlffi_get_field_float(player, "speed", 1.0);
bool active = hlffi_get_field_bool(player, "active", false);

// Writing (one line!)
hlffi_set_field_int(vm, player, "health", 100);
hlffi_set_field_float(vm, player, "speed", 2.5);
hlffi_set_field_bool(vm, player, "active", true);
```

---

## Calling Instance Methods

Call methods on your Haxe objects.

### C Side

```c
// Call player.takeDamage(25)
hlffi_value* damage = hlffi_value_int(vm, 25);
hlffi_call_method(player, "takeDamage", 1, &damage);
hlffi_value_free(damage);

// Call player.getHealth()
hlffi_value* hp = hlffi_call_method(player, "getHealth", 0, NULL);
int health = hlffi_value_as_int(hp, 0);
printf("Current health: %d\n", health);
hlffi_value_free(hp);
```

---

## Complete Example

Here's a full example putting it all together:

### Haxe: game.hx

```haxe
class Player
{
    public var name:String;
    public var health:Int;
    public var score:Int;

    public function new(playerName:String)
    {
        this.name = playerName;
        this.health = 100;
        this.score = 0;
    }

    public function takeDamage(amount:Int):Bool
    {
        health -= amount;
        if (health <= 0)
        {
            health = 0;
            trace('$name died!');
            return false;  // Dead
        }
        trace('$name has $health HP left');
        return true;  // Alive
    }

    public function addScore(points:Int):Void
    {
        score += points;
        trace('$name scored $points points! Total: $score');
    }
}

class Game
{
    public static var currentPlayer:Player;

    public static function createPlayer(name:String):Void
    {
        currentPlayer = new Player(name);
        trace('Created player: $name');
    }

    public static function getPlayerHealth():Int
    {
        return currentPlayer != null ? currentPlayer.health : 0;
    }
}

class Main
{
    public static function main()
    {
        trace("Game module loaded");
    }
}
```

Compile:
```bash
haxe -main Main -hl game.hl game.hx
```

### C: main.c

```c
#include <stdio.h>
#include "hlffi.h"

int main()
{
    // Setup VM
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Create a player using static method
    hlffi_value* name = hlffi_value_string(vm, "Hero");
    hlffi_value* args[] = {name};
    hlffi_call_static(vm, "Game", "createPlayer", 1, args);
    hlffi_value_free(name);

    // Get player health
    hlffi_value* hp_val = hlffi_call_static(vm, "Game", "getPlayerHealth", 0, NULL);
    int health = hlffi_value_as_int(hp_val, 0);
    printf("Initial health: %d\n", health);
    hlffi_value_free(hp_val);

    // Create another player object directly
    hlffi_value* name2 = hlffi_value_string(vm, "Villain");
    hlffi_value* villain = hlffi_new(vm, "Player", 1, &name2);
    hlffi_value_free(name2);

    // Damage the villain
    hlffi_value* damage = hlffi_value_int(vm, 30);
    hlffi_value* alive = hlffi_call_method(villain, "takeDamage", 1, &damage);
    printf("Villain alive: %s\n", hlffi_value_as_bool(alive, true) ? "yes" : "no");
    hlffi_value_free(damage);
    hlffi_value_free(alive);

    // Add score
    hlffi_value* points = hlffi_value_int(vm, 100);
    hlffi_call_method(villain, "addScore", 1, &points);
    hlffi_value_free(points);

    // Read villain's score
    int score = hlffi_get_field_int(villain, "score", 0);
    printf("Villain score: %d\n", score);

    // Cleanup
    hlffi_value_free(villain);
    hlffi_close(vm);
    hlffi_destroy(vm);

    return 0;
}
```

**Output:**
```
Game module loaded
Created player: Hero
Initial health: 100
Villain took 30 damage, health: 70
Villain alive: yes
Villain scored 100 points! Total: 100
Villain score: 100
```

---

## Memory Rules

**Always remember these rules:**

| What you get | How to free it |
|--------------|----------------|
| `hlffi_value*` from any function | `hlffi_value_free(val)` |
| `char*` from `hlffi_value_as_string()` | `free(str)` |
| Primitives (int, float, bool) | Nothing to free |

**Example:**

```c
// Values need hlffi_value_free()
hlffi_value* val = hlffi_value_int(vm, 42);
// ... use val ...
hlffi_value_free(val);

// Strings from as_string need free()
hlffi_value* str_val = hlffi_get_field(obj, "name");
char* str = hlffi_value_as_string(str_val);
printf("%s\n", str);
free(str);                    // Free the string
hlffi_value_free(str_val);    // Free the value
```

---

## Quick Reference

### Calling Static Methods

```c
// No args, no return
hlffi_call_static(vm, "Class", "method", 0, NULL);

// With args and return
hlffi_value* args[] = {arg1, arg2};
hlffi_value* result = hlffi_call_static(vm, "Class", "method", 2, args);
```

### Static Variables

```c
// Read
hlffi_value* val = hlffi_get_static_field(vm, "Class", "field");

// Write
hlffi_set_static_field(vm, "Class", "field", val);
```

### Creating Objects

```c
hlffi_value* obj = hlffi_new(vm, "ClassName", argc, args);
```

### Instance Fields

```c
// Read
hlffi_value* val = hlffi_get_field(obj, "field");
int i = hlffi_get_field_int(obj, "field", default);

// Write
hlffi_set_field(obj, "field", val);
hlffi_set_field_int(vm, obj, "field", value);
```

### Instance Methods

```c
hlffi_value* result = hlffi_call_method(obj, "method", argc, args);
```

---

## What's Next?

Now that you can call Haxe code, let's learn about:
- Working with arrays and maps
- Handling bytes and binary data
- Understanding enums and abstracts

**[Next: Data Exchange →](GUIDE_03_DATA_EXCHANGE.md)**
