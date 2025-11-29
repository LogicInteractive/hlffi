# HLFFI API Reference - Instance Members

**[← Static Members](API_08_STATIC_MEMBERS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Arrays →](API_10_ARRAYS.md)**

Create objects and access instance fields/methods.

---

## Object Lifecycle

1. Create with `hlffi_new()` - automatically GC-rooted
2. Use fields/methods
3. Free with `hlffi_value_free()` - removes GC root

---

## Quick Reference

### Core API

| Function | Purpose |
|----------|---------|
| `hlffi_new(vm, "Player", argc, argv)` | Create instance (call constructor) |
| `hlffi_get_field(obj, "health")` | Get instance field |
| `hlffi_set_field(obj, "health", val)` | Set instance field |
| `hlffi_call_method(obj, "move", argc, argv)` | Call instance method |
| `hlffi_is_instance_of(obj, "Player")` | Type check |

### Convenience API (70% code reduction)

| Function | Purpose |
|----------|---------|
| `hlffi_get_field_int/float/bool/string()` | Get field as native type |
| `hlffi_set_field_int/float/bool/string()` | Set field from native type |
| `hlffi_call_method_void/int/float/bool/string()` | Call method, typed return |

**Complete Guide:** See `docs/PHASE4_INSTANCE_MEMBERS.md`

---

## Example (Generic API)

```c
// Create instance:
hlffi_value* name_arg = hlffi_value_string(vm, "Hero");
hlffi_value* player = hlffi_new(vm, "Player", 1, &name_arg);
hlffi_value_free(name_arg);

// Get field:
hlffi_value* hp_val = hlffi_get_field(player, "health");
int health = hlffi_value_as_int(hp_val, 0);
hlffi_value_free(hp_val);
printf("Health: %d\n", health);

// Set field:
hlffi_value* new_hp = hlffi_value_int(vm, 100);
hlffi_set_field(player, "health", new_hp);
hlffi_value_free(new_hp);

// Call method:
hlffi_value* damage = hlffi_value_int(vm, 25);
hlffi_call_method(player, "takeDamage", 1, &damage);
hlffi_value_free(damage);

// Cleanup:
hlffi_value_free(player);
```

---

## Example (Convenience API - Recommended)

```c
// Create instance:
hlffi_value* name_arg = hlffi_value_string(vm, "Hero");
hlffi_value* player = hlffi_new(vm, "Player", 1, &name_arg);
hlffi_value_free(name_arg);

// Get field (one line):
int health = hlffi_get_field_int(player, "health", 0);
printf("Health: %d\n", health);

// Set field (one line):
hlffi_set_field_int(vm, player, "health", 100);

// Call method with void return:
hlffi_value* damage = hlffi_value_int(vm, 25);
hlffi_call_method_void(player, "takeDamage", 1, &damage);
hlffi_value_free(damage);

// Call method with int return:
int current_hp = hlffi_call_method_int(player, "getHealth", 0, NULL, 0);
printf("Current health: %d\n", current_hp);

// Cleanup:
hlffi_value_free(player);
```

---

## Code Comparison

### Before (Generic API):
```c
hlffi_value* hp = hlffi_get_field(player, "health");
int health = hlffi_value_as_int(hp, 0);
hlffi_value_free(hp);
```

### After (Convenience API):
```c
int health = hlffi_get_field_int(player, "health", 0);
```

**70% less code!**

---

## Complete Example

```c
#include "hlffi.h"

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);
    
    // Create player with name:
    hlffi_value* name = hlffi_value_string(vm, "Warrior");
    hlffi_value* player = hlffi_new(vm, "Player", 1, &name);
    hlffi_value_free(name);
    
    // Set initial health:
    hlffi_set_field_int(vm, player, "health", 100);
    
    // Get and display health:
    int hp = hlffi_get_field_int(player, "health", 0);
    printf("Health: %d\n", hp);
    
    // Apply damage:
    hlffi_value* dmg = hlffi_value_int(vm, 25);
    hlffi_call_method_void(player, "takeDamage", 1, &dmg);
    hlffi_value_free(dmg);
    
    // Check health after damage:
    hp = hlffi_call_method_int(player, "getHealth", 0, NULL, 0);
    printf("Health after damage: %d\n", hp);
    
    // Type check:
    if (hlffi_is_instance_of(player, "Player")) {
        printf("Confirmed: is a Player\n");
    }
    
    // Cleanup:
    hlffi_value_free(player);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
class Player {
    public var health:Int;
    public var name:String;
    
    public function new(name:String) {
        this.name = name;
        this.health = 100;
    }
    
    public function takeDamage(amount:Int):Void {
        health -= amount;
        if (health < 0) health = 0;
    }
    
    public function getHealth():Int {
        return health;
    }
}
```

---

## Best Practices

### 1. Use Convenience API

```c
// ✅ GOOD - Concise
int hp = hlffi_get_field_int(player, "health", 0);

// ❌ Verbose (but still works)
hlffi_value* val = hlffi_get_field(player, "health");
int hp = hlffi_value_as_int(val, 0);
hlffi_value_free(val);
```

### 2. Free Objects When Done

```c
// ✅ GOOD
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
// ... use player ...
hlffi_value_free(player);  // Remove GC root

// ❌ BAD - Memory leak (GC root never removed)
hlffi_value* player = hlffi_new(vm, "Player", 0, NULL);
// ... use player ...
// Missing: hlffi_value_free(player);
```

### 3. Type Check Before Casting

```c
// ✅ GOOD
if (hlffi_is_instance_of(obj, "Player")) {
    // Safe to use as Player
}

// ❌ BAD - No type check
// Assume obj is Player without checking
```

---

**[← Static Members](API_08_STATIC_MEMBERS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Arrays →](API_10_ARRAYS.md)**
