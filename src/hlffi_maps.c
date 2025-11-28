/**
 * Map Support for HLFFI
 *
 * Practical approach: Work with Map objects from Haxe via method calls
 * Maps are created in Haxe, manipulated in C, passed back to Haxe
 */

#include "hlffi_internal.h"
#include <string.h>

/* Map operations via instance method calls */

hlffi_value* hlffi_map_new(hlffi_vm* vm, hl_type* key_type, hl_type* value_type) {
    if (!vm) return NULL;

    /* Maps should be created in Haxe for now.
     * Creating from C requires complex type instantiation. */
    return NULL;
}

bool hlffi_map_set(hlffi_vm* vm, hlffi_value* map, hlffi_value* key, hlffi_value* value) {
    if (!vm || !map || !key) return false;

    /* Call map.set(key, value) via instance method */
    hlffi_value* args[] = {key, value};
    hlffi_value* result = hlffi_call_method(map, "set", 2, args);

    if (!result) return false;
    hlffi_value_free(result);
    return true;
}

hlffi_value* hlffi_map_get(hlffi_vm* vm, hlffi_value* map, hlffi_value* key) {
    if (!vm || !map || !key) return NULL;

    /* Call map.get(key) */
    hlffi_value* args[] = {key};
    return hlffi_call_method(map, "get", 1, args);
}

bool hlffi_map_exists(hlffi_vm* vm, hlffi_value* map, hlffi_value* key) {
    if (!vm || !map || !key) return false;

    /* Call map.exists(key) */
    hlffi_value* args[] = {key};
    hlffi_value* result = hlffi_call_method(map, "exists", 1, args);

    if (!result) return false;

    /* Extract boolean result using standard helper */
    bool exists = hlffi_value_as_bool(result, false);
    hlffi_value_free(result);
    return exists;
}

bool hlffi_map_remove(hlffi_vm* vm, hlffi_value* map, hlffi_value* key) {
    if (!vm || !map || !key) return false;

    /* Call map.remove(key) */
    hlffi_value* args[] = {key};
    hlffi_value* result = hlffi_call_method(map, "remove", 1, args);

    if (!result) return false;

    /* Extract boolean result using standard helper */
    bool removed = hlffi_value_as_bool(result, false);
    hlffi_value_free(result);
    return removed;
}

hlffi_value* hlffi_map_keys(hlffi_vm* vm, hlffi_value* map) {
    if (!vm || !map) return NULL;

    /* Call map.keys() - returns iterator */
    return hlffi_call_method(map, "keys", 0, NULL);
}

hlffi_value* hlffi_map_values(hlffi_vm* vm, hlffi_value* map) {
    if (!vm || !map) return NULL;

    /* Call map.iterator() - returns value iterator */
    return hlffi_call_method(map, "iterator", 0, NULL);
}

int hlffi_map_size(hlffi_value* map) {
    if (!map) return -1;

    /* Maps don't have direct size(), need to iterate or use Lambda.count()
     * Return -1 to indicate not directly available */
    return -1;
}

bool hlffi_map_clear(hlffi_value* map) {
    if (!map) return false;

    /* Map.clear() exists in some Haxe versions
     * Not implemented for now */
    return false;
}
