# HLFFI API Reference - Static Members

**[← Values](API_07_VALUES.md)** | **[Back to Index](API_REFERENCE.md)** | **[Instance Members →](API_09_INSTANCE_MEMBERS.md)**

Access static fields and methods on Haxe classes.

---

## Precondition

**Must call `hlffi_call_entry()` before accessing static members** (initializes globals).

---

## Quick Reference

| Function | Purpose |
|----------|---------|
| `hlffi_get_static_field(vm, "Game", "score")` | Get static field |
| `hlffi_set_static_field(vm, "Game", "score", val)` | Set static field |
| `hlffi_call_static(vm, "Game", "start", argc, argv)` | Call static method |

**Complete Guide:** See `docs/PHASE3_COMPLETE.md` (1000+ lines)

---

## Example

```c
// Get static field:
hlffi_value* score_val = hlffi_get_static_field(vm, "Game", "highScore");
int score = hlffi_value_as_int(score_val, 0);
hlffi_value_free(score_val);
printf("High score: %d\n", score);

// Set static field:
hlffi_value* new_score = hlffi_value_int(vm, 9000);
hlffi_set_static_field(vm, "Game", "highScore", new_score);
hlffi_value_free(new_score);

// Call static method (no args):
hlffi_call_static(vm, "Game", "initialize", 0, NULL);

// Call static method (with args):
hlffi_value* level = hlffi_value_int(vm, 5);
hlffi_value* args[] = {level};
hlffi_call_static(vm, "Game", "loadLevel", 1, args);
hlffi_value_free(level);
```

---

## Functions

### `hlffi_get_static_field()`

**Signature:**
```c
hlffi_value* hlffi_get_static_field(hlffi_vm* vm, const char* class_name, const char* field_name)
```

**Returns:** Field value, or NULL on error

**Example:**
```c
hlffi_value* val = hlffi_get_static_field(vm, "Config", "maxPlayers");
if (!val) {
    fprintf(stderr, "Field not found: %s\n", hlffi_get_error(vm));
}
```

---

### `hlffi_set_static_field()`

**Signature:**
```c
hlffi_error_code hlffi_set_static_field(
    hlffi_vm* vm,
    const char* class_name,
    const char* field_name,
    hlffi_value* value
)
```

**Returns:** `HLFFI_OK` or error code

---

### `hlffi_call_static()`

**Signature:**
```c
hlffi_value* hlffi_call_static(
    hlffi_vm* vm,
    const char* class_name,
    const char* method_name,
    int argc,
    hlffi_value** argv
)
```

**Returns:** Return value, or NULL on error/void return

**Example:**
```c
// Void method:
hlffi_call_static(vm, "Logger", "log", 0, NULL);

// Method with return:
hlffi_value* result = hlffi_call_static(vm, "Math", "add", 2, args);
int sum = hlffi_value_as_int(result, 0);
hlffi_value_free(result);
```

---

## Complete Example

```c
#include "hlffi.h"

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);  // MUST call before accessing statics
    
    // Get config:
    hlffi_value* width_val = hlffi_get_static_field(vm, "Config", "screenWidth");
    int width = hlffi_value_as_int(width_val, 800);
    hlffi_value_free(width_val);
    
    // Set player name:
    hlffi_value* name = hlffi_value_string(vm, "Hero");
    hlffi_set_static_field(vm, "Game", "playerName", name);
    hlffi_value_free(name);
    
    // Call game start:
    hlffi_call_static(vm, "Game", "start", 0, NULL);
    
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Config {
    public static var screenWidth:Int = 1920;
}

class Game {
    public static var playerName:String = "Default";
    
    public static function start():Void {
        trace('Starting game for: $playerName');
    }
}
```

---

**[← Values](API_07_VALUES.md)** | **[Back to Index](API_REFERENCE.md)** | **[Instance Members →](API_09_INSTANCE_MEMBERS.md)**
