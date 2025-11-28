# HLFFI API Reference - Bytes (Binary Data)

**[← Maps](API_11_MAPS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Enums →](API_13_ENUMS.md)**

Work with binary data and byte buffers (Haxe `haxe.io.Bytes`).

---

## Quick Reference

### Creation

| Function | Purpose |
|----------|---------|
| `hlffi_bytes_new(vm, size)` | Allocate empty byte buffer |
| `hlffi_bytes_from_data(vm, ptr, size)` | Create from C data (copies) |
| `hlffi_bytes_from_string(vm, str)` | Create from C string |

### Access

| Function | Purpose |
|----------|---------|
| `hlffi_bytes_get_ptr(bytes)` | Get direct pointer (zero-copy) |
| `hlffi_bytes_get_length(bytes)` | Get byte count |
| `hlffi_bytes_get(bytes, idx)` | Get single byte |
| `hlffi_bytes_set(bytes, idx, val)` | Set single byte |

### Operations

| Function | Purpose |
|----------|---------|
| `hlffi_bytes_blit(dst, dpos, src, spos, len)` | Copy bytes |
| `hlffi_bytes_compare(a, apos, b, bpos, len)` | Compare bytes |
| `hlffi_bytes_fill(bytes, pos, len, val)` | Fill with value |
| `hlffi_bytes_to_string(bytes, len)` | Convert to C string |

**Complete Guide:** See `docs/PHASE4_INSTANCE_MEMBERS.md`

---

## Creating Bytes

### Allocate Empty Buffer

**Signature:**
```c
hlffi_value* hlffi_bytes_new(hlffi_vm* vm, int size)
```

**Returns:** New byte buffer (zero-initialized), or NULL on error

**Example:**
```c
hlffi_value* buf = hlffi_bytes_new(vm, 1024);  // 1 KB buffer
```

---

### Create from C Data

**Signature:**
```c
hlffi_value* hlffi_bytes_from_data(hlffi_vm* vm, const void* data, int size)
```

**Description:** Copy C data into new Haxe Bytes.

**Example:**
```c
unsigned char data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"
hlffi_value* bytes = hlffi_bytes_from_data(vm, data, 5);
```

---

### Create from C String

**Signature:**
```c
hlffi_value* hlffi_bytes_from_string(hlffi_vm* vm, const char* str)
```

**Description:** Copy C string (excluding null terminator) into Bytes.

**Example:**
```c
hlffi_value* bytes = hlffi_bytes_from_string(vm, "Hello");
// Length: 5 (no null terminator)
```

---

## Accessing Data

### Get Direct Pointer (Zero-Copy)

**Signature:**
```c
void* hlffi_bytes_get_ptr(hlffi_value* bytes)
```

**Returns:** Direct pointer to byte buffer, or NULL if not bytes

**Example:**
```c
hlffi_value* bytes = hlffi_bytes_new(vm, 10);
unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(bytes);

if (ptr) {
    ptr[0] = 0xFF;  // Modify in-place
    ptr[1] = 0xAA;
}
```

---

### Get Length

**Signature:**
```c
int hlffi_bytes_get_length(hlffi_value* bytes)
```

**Returns:** Byte count, or -1 if not bytes

**Example:**
```c
int len = hlffi_bytes_get_length(bytes);
printf("Buffer size: %d bytes\n", len);
```

---

### Get Single Byte

**Signature:**
```c
int hlffi_bytes_get(hlffi_value* bytes, int index)
```

**Returns:** Byte value (0-255), or -1 if out of bounds

**Example:**
```c
int byte = hlffi_bytes_get(bytes, 0);
if (byte >= 0) {
    printf("First byte: 0x%02X\n", byte);
}
```

---

### Set Single Byte

**Signature:**
```c
bool hlffi_bytes_set(hlffi_value* bytes, int index, int value)
```

**Parameters:**
- `value` - Byte value (0-255, higher bits ignored)

**Returns:** `true` on success, `false` if out of bounds

**Example:**
```c
hlffi_bytes_set(bytes, 0, 0xFF);
```

---

## Operations

### Copy Bytes (Blit)

**Signature:**
```c
bool hlffi_bytes_blit(hlffi_value* dst, int dst_pos, hlffi_value* src, int src_pos, int len)
```

**Description:** Copy `len` bytes from `src[src_pos..]` to `dst[dst_pos..]`.

**Returns:** `true` on success, `false` if out of bounds

**Example:**
```c
hlffi_value* src = hlffi_bytes_from_string(vm, "HelloWorld");
hlffi_value* dst = hlffi_bytes_new(vm, 5);

// Copy "World" (5 bytes from src[5..]):
hlffi_bytes_blit(dst, 0, src, 5, 5);

char* str = hlffi_bytes_to_string(dst, 5);
printf("%s\n", str);  // "World"
free(str);
```

---

### Compare Bytes

**Signature:**
```c
int hlffi_bytes_compare(hlffi_value* a, int a_pos, hlffi_value* b, int b_pos, int len)
```

**Returns:**
- `< 0` if `a` < `b`
- `0` if equal
- `> 0` if `a` > `b`
- `-1` on error

**Example:**
```c
hlffi_value* a = hlffi_bytes_from_string(vm, "abc");
hlffi_value* b = hlffi_bytes_from_string(vm, "abd");

int cmp = hlffi_bytes_compare(a, 0, b, 0, 3);
if (cmp < 0) {
    printf("a < b\n");
}
```

---

### Fill with Value

**Signature:**
```c
bool hlffi_bytes_fill(hlffi_value* bytes, int pos, int len, int value)
```

**Description:** Fill `len` bytes starting at `pos` with `value`.

**Example:**
```c
hlffi_value* bytes = hlffi_bytes_new(vm, 10);
hlffi_bytes_fill(bytes, 0, 10, 0xFF);  // Fill all with 0xFF
```

---

### Convert to String

**Signature:**
```c
char* hlffi_bytes_to_string(hlffi_value* bytes, int length)
```

**Description:** Copy first `length` bytes to C string (**caller must free()**).

**Returns:** Null-terminated string, or NULL on error

**Example:**
```c
hlffi_value* bytes = hlffi_bytes_from_string(vm, "Hello");
char* str = hlffi_bytes_to_string(bytes, 5);
printf("%s\n", str);  // "Hello"
free(str);  // MUST free
```

---

## Complete Example

```c
#include "hlffi.h"

int main() {
    hlffi_vm* vm = hlffi_create();
    hlffi_init(vm, 0, NULL);
    hlffi_load_file(vm, "game.hl");
    hlffi_call_entry(vm);

    // Create buffer from data:
    unsigned char data[] = {0x48, 0x45, 0x4C, 0x4C, 0x4F};  // "HELLO"
    hlffi_value* bytes = hlffi_bytes_from_data(vm, data, 5);

    // Get length:
    int len = hlffi_bytes_get_length(bytes);
    printf("Length: %d\n", len);  // 5

    // Read bytes:
    printf("Bytes: ");
    for (int i = 0; i < len; i++) {
        int byte = hlffi_bytes_get(bytes, i);
        printf("0x%02X ", byte);
    }
    printf("\n");

    // Zero-copy access:
    unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(bytes);
    if (ptr) {
        ptr[0] = 0x68;  // 'H' -> 'h'
    }

    // Convert to string:
    char* str = hlffi_bytes_to_string(bytes, len);
    printf("As string: %s\n", str);  // "hELLO"
    free(str);

    // Copy operation:
    hlffi_value* dst = hlffi_bytes_new(vm, 2);
    hlffi_bytes_blit(dst, 0, bytes, 1, 2);  // Copy "EL"

    str = hlffi_bytes_to_string(dst, 2);
    printf("Copied: %s\n", str);  // "EL"
    free(str);

    // Cleanup:
    hlffi_value_free(bytes);
    hlffi_value_free(dst);
    hlffi_destroy(vm);
    return 0;
}
```

**Haxe Side:**
```haxe
import haxe.io.Bytes;

class Main {
    public static function main() {
        var bytes = Bytes.alloc(10);
        bytes.set(0, 0xFF);
        trace('Byte 0: ${bytes.get(0)}');
    }
}
```

---

## Best Practices

### 1. Use Zero-Copy Access for Performance

```c
// ✅ GOOD - Direct memory access
unsigned char* ptr = (unsigned char*)hlffi_bytes_get_ptr(bytes);
for (int i = 0; i < len; i++) {
    ptr[i] = i;  // Fast
}

// ❌ SLOWER - Individual calls
for (int i = 0; i < len; i++) {
    hlffi_bytes_set(bytes, i, i);  // Function call overhead
}
```

### 2. Free Strings from `hlffi_bytes_to_string()`

```c
// ✅ GOOD
char* str = hlffi_bytes_to_string(bytes, 5);
printf("%s\n", str);
free(str);  // MUST free

// ❌ BAD - Memory leak
char* str = hlffi_bytes_to_string(bytes, 5);
printf("%s\n", str);
// Missing: free(str);
```

### 3. Check Bounds for Operations

```c
// ✅ GOOD - Check return value
if (!hlffi_bytes_blit(dst, 0, src, 5, 10)) {
    fprintf(stderr, "Blit failed - out of bounds\n");
}

// ❌ BAD - Ignore errors
hlffi_bytes_blit(dst, 0, src, 5, 10);  // Could fail silently
```

---

**[← Maps](API_11_MAPS.md)** | **[Back to Index](API_REFERENCE.md)** | **[Enums →](API_13_ENUMS.md)**
