# HLFFI API Reference - Value System

**[← Type System](API_06_TYPE_SYSTEM.md)** | **[Back to Index](API_REFERENCE.md)** | **[Static Members →](API_08_STATIC_MEMBERS.md)**

The value system provides boxing/unboxing for converting between C and Haxe types.

---

## Core Principle

**`hlffi_value` is for CONVERSION, not storage.**

```c
// ✅ CORRECT: Temporary conversion
hlffi_value* temp = hlffi_value_int(vm, 100);
hlffi_call_method(obj, "setHealth", 1, &temp);
hlffi_value_free(temp);  // Free immediately

// ✅ CORRECT: Extract to C type for storage
hlffi_value* hp_val = hlffi_get_field(obj, "health");
int health = hlffi_value_as_int(hp_val, 0);  // Copy to C int
hlffi_value_free(hp_val);
// Now 'health' is safe to store

// ❌ WRONG: Storing non-rooted value
struct Data
{
data->temp = hlffi_value_int(vm, 100);  // NOT GC-rooted!
```

---

## Boxing (C → Haxe)

| Function | C Type | Haxe Type |
|----------|--------|-----------|
| `hlffi_value_int(vm, 42)` | `int` | `Int` |
| `hlffi_value_float(vm, 3.14)` | `double` | `Float` (F64) |
| `hlffi_value_f32(vm, 3.14f)` | `float` | `hl.F32` |
| `hlffi_value_bool(vm, true)` | `bool` | `Bool` |
| `hlffi_value_string(vm, "hi")` | `const char*` (UTF-8) | `String` (UTF-16) |
| `hlffi_value_null(vm)` | - | `null` |

**Example:**
```c
hlffi_value* score = hlffi_value_int(vm, 1000);
hlffi_value* name = hlffi_value_string(vm, "Player1");
hlffi_value* active = hlffi_value_bool(vm, true);

// Use in function call:
hlffi_value* args[] = {score, name, active};
hlffi_call_static(vm, "Game", "initPlayer", 3, args);

// Free all:
hlffi_value_free(score);
hlffi_value_free(name);
hlffi_value_free(active);
```

---

## Unboxing (Haxe → C)

| Function | Returns | Notes |
|----------|---------|-------|
| `hlffi_value_as_int(val, fallback)` | `int` | Returns fallback if not int |
| `hlffi_value_as_float(val, fallback)` | `double` | F64 |
| `hlffi_value_as_f32(val, fallback)` | `float` | F32 |
| `hlffi_value_as_bool(val, fallback)` | `bool` | Returns fallback if not bool |
| `hlffi_value_as_string(val)` | `char*` | **Caller must free()** |
| `hlffi_value_is_null(val)` | `bool` | Check for null |

**Example:**
```c
hlffi_value* result = hlffi_call_static(vm, "Game", "getScore", 0, NULL);
int score = hlffi_value_as_int(result, 0);  // Fallback to 0
printf("Score: %d\n", score);
hlffi_value_free(result);

// Strings need special handling:
hlffi_value* name_val = hlffi_get_field(player, "name");
char* name = hlffi_value_as_string(name_val);  // Allocates string
hlffi_value_free(name_val);  // Can free value immediately
printf("Name: %s\n", name);
free(name);  // MUST free the string!
```

---

## Memory Management

### `hlffi_value_free()`

**Signature:**
```c
void hlffi_value_free(hlffi_value* value)
```

**Description:**
Frees a value wrapper. Removes GC root if present. **Always use this instead of free()**.

**Safe to:**
- Call with NULL
- Call multiple times (idempotent)

**Example:**
```c
hlffi_value* val = hlffi_value_int(vm, 42);
hlffi_value_free(val);  // Always free when done
```

---

## Memory Ownership Rules

| Source | GC-Rooted? | Must Free? |
|--------|------------|------------|
| `hlffi_value_int/float/bool/string()` | ❌ No | ✅ Yes |
| `hlffi_value_null()` | ❌ No | ✅ Yes |
| `hlffi_new()` | ✅ Yes | ✅ Yes |
| `hlffi_get_field()` | ❌ No | ✅ Yes |
| `hlffi_call_method()` | ❌ No | ✅ Yes (if non-NULL) |
| `hlffi_call_static()` | ❌ No | ✅ Yes (if non-NULL) |
| `hlffi_value_as_string()` return | N/A | ✅ Yes (`free()`, not `hlffi_value_free()`) |

**Rule of Thumb:** If you create it or get it from HLFFI, you must free it.

---

## Best Practices

### 1. Free Immediately

```c
// ✅ GOOD - Free as soon as done
hlffi_value* arg = hlffi_value_int(vm, 100);
hlffi_call_static(vm, "Game", "setScore", 1, &arg);
hlffi_value_free(arg);

// ❌ BAD - Holding on too long
hlffi_value* arg = hlffi_value_int(vm, 100);
// ... lots of code ...
hlffi_value_free(arg);  // Risk of forgetting
```

### 2. Extract Primitives for Storage

```c
// ✅ GOOD - Extract to C type
hlffi_value* hp = hlffi_get_field(player, "health");
int health = hlffi_value_as_int(hp, 100);
hlffi_value_free(hp);
// Store 'health' (C int) anywhere

// ❌ BAD - Store value wrapper
struct Player
{
```

### 3. Only Store GC-Rooted Values

```c
// ✅ GOOD - hlffi_new() is rooted
struct Game
{
game->player = hlffi_new(vm, "Player", 0, NULL);  // GC-rooted, safe to store

// ❌ BAD - Not rooted
game->player = hlffi_get_field(otherobj, "player");  // NOT rooted, NOT safe!
```

---

**[← Type System](API_06_TYPE_SYSTEM.md)** | **[Back to Index](API_REFERENCE.md)** | **[Static Members →](API_08_STATIC_MEMBERS.md)**