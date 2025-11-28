/* ========== Map Support ========== */

/* Map helper functions - maps are complex in HashLink due to template-like implementation */

/* Forward declarations for HashLink map functions */
extern void* hl_alloc_intmap();
extern void hl_intmap_set(void* map, int key, vdynamic* value);
extern vdynamic* hl_intmap_get(void* map, int key);
extern bool hl_intmap_exists(void* map, int key);
extern bool hl_intmap_remove(void* map, int key);
extern varray* hl_intmap_keys(void* map);
extern varray* hl_intmap_values(void* map);
extern int hl_intmap_size(void* map);
extern void hl_intmap_clear(void* map);

extern void* hl_alloc_strbytesmap();
extern void hl_strbytesmap_set(void* map, vbyte* key, vdynamic* value);
extern vdynamic* hl_strbytesmap_get(void* map, vbyte* key);
extern bool hl_strbytesmap_exists(void* map, vbyte* key);
extern bool hl_strbytesmap_remove(void* map, vbyte* key);
extern varray* hl_strbytesmap_keys(void* map);
extern varray* hl_strbytesmap_values(void* map);
extern int hl_strbytesmap_size(void* map);
extern void hl_strbytesmap_clear(void* map);

/* Map type identifier stored in the hlffi_value */
typedef enum {
    HLFFI_MAP_INT,     /* Map<Int, T> */
    HLFFI_MAP_STRING,  /* Map<String, T> */
    HLFFI_MAP_OBJECT   /* Map<Object, T> */
} hlffi_map_type;

/* Extended map wrapper to track type */
typedef struct {
    vdynamic header;   /* Must be first for vdynamic* compatibility */
    void* map_ptr;     /* Actual HashLink map pointer */
    hlffi_map_type type;
} hlffi_map_wrapper;

hlffi_value* hlffi_map_new(hlffi_vm* vm, hl_type* key_type, hl_type* value_type) {
    if (!vm || !key_type) return NULL;

    HLFFI_UPDATE_STACK_TOP();

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)hl_gc_alloc_raw(sizeof(hlffi_map_wrapper));
    if (!wrapper) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate map wrapper");
        return NULL;
    }

    /* Determine map type based on key_type */
    if (key_type->kind == HI32 || key_type->kind == HI64) {
        /* IntMap */
        wrapper->type = HLFFI_MAP_INT;
        wrapper->map_ptr = hl_alloc_intmap();
    } else if (key_type->kind == HBYTES) {
        /* StringMap */
        wrapper->type = HLFFI_MAP_STRING;
        wrapper->map_ptr = hl_alloc_strbytesmap();
    } else {
        /* ObjectMap or other - not yet implemented */
        set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED, "Map type not yet supported (only Int and String keys)");
        return NULL;
    }

    if (!wrapper->map_ptr) {
        set_error(vm, HLFFI_ERROR_OUT_OF_MEMORY, "Failed to allocate map");
        return NULL;
    }

    /* Wrap in hlffi_value */
    hlffi_value* result = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!result) return NULL;
    result->hl_value = (vdynamic*)wrapper;
    result->is_rooted = false;

    return result;
}

bool hlffi_map_set(hlffi_vm* vm, hlffi_value* map, hlffi_value* key, hlffi_value* value) {
    if (!vm || !map || !map->hl_value || !key || !key->hl_value) return false;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;

    HLFFI_UPDATE_STACK_TOP();

    switch (wrapper->type) {
        case HLFFI_MAP_INT: {
            int k = hlffi_value_as_int(key, 0);
            vdynamic* v = value ? value->hl_value : NULL;
            hl_intmap_set(wrapper->map_ptr, k, v);
            return true;
        }
        case HLFFI_MAP_STRING: {
            char* k_str = hlffi_value_as_string(key);
            if (!k_str) return false;
            vbyte* k = (vbyte*)k_str;  /* Convert UTF-8 to bytes */
            vdynamic* v = value ? value->hl_value : NULL;
            hl_strbytesmap_set(wrapper->map_ptr, k, v);
            free(k_str);
            return true;
        }
        default:
            set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED, "Map type not supported");
            return false;
    }
}

hlffi_value* hlffi_map_get(hlffi_vm* vm, hlffi_value* map, hlffi_value* key) {
    if (!vm || !map || !map->hl_value || !key || !key->hl_value) return NULL;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;
    vdynamic* result = NULL;

    switch (wrapper->type) {
        case HLFFI_MAP_INT: {
            int k = hlffi_value_as_int(key, 0);
            result = hl_intmap_get(wrapper->map_ptr, k);
            break;
        }
        case HLFFI_MAP_STRING: {
            char* k_str = hlffi_value_as_string(key);
            if (!k_str) return NULL;
            vbyte* k = (vbyte*)k_str;
            result = hl_strbytesmap_get(wrapper->map_ptr, k);
            free(k_str);
            break;
        }
        default:
            set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED, "Map type not supported");
            return NULL;
    }

    if (!result) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;
    wrapped->hl_value = result;
    wrapped->is_rooted = false;
    return wrapped;
}

bool hlffi_map_exists(hlffi_vm* vm, hlffi_value* map, hlffi_value* key) {
    if (!vm || !map || !map->hl_value || !key || !key->hl_value) return false;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;

    switch (wrapper->type) {
        case HLFFI_MAP_INT: {
            int k = hlffi_value_as_int(key, 0);
            return hl_intmap_exists(wrapper->map_ptr, k);
        }
        case HLFFI_MAP_STRING: {
            char* k_str = hlffi_value_as_string(key);
            if (!k_str) return false;
            vbyte* k = (vbyte*)k_str;
            bool exists = hl_strbytesmap_exists(wrapper->map_ptr, k);
            free(k_str);
            return exists;
        }
        default:
            return false;
    }
}

bool hlffi_map_remove(hlffi_vm* vm, hlffi_value* map, hlffi_value* key) {
    if (!vm || !map || !map->hl_value || !key || !key->hl_value) return false;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;

    HLFFI_UPDATE_STACK_TOP();

    switch (wrapper->type) {
        case HLFFI_MAP_INT: {
            int k = hlffi_value_as_int(key, 0);
            return hl_intmap_remove(wrapper->map_ptr, k);
        }
        case HLFFI_MAP_STRING: {
            char* k_str = hlffi_value_as_string(key);
            if (!k_str) return false;
            vbyte* k = (vbyte*)k_str;
            bool removed = hl_strbytesmap_remove(wrapper->map_ptr, k);
            free(k_str);
            return removed;
        }
        default:
            return false;
    }
}

hlffi_value* hlffi_map_keys(hlffi_vm* vm, hlffi_value* map) {
    if (!vm || !map || !map->hl_value) return NULL;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;
    varray* keys = NULL;

    switch (wrapper->type) {
        case HLFFI_MAP_INT:
            keys = hl_intmap_keys(wrapper->map_ptr);
            break;
        case HLFFI_MAP_STRING:
            keys = hl_strbytesmap_keys(wrapper->map_ptr);
            break;
        default:
            set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED, "Map type not supported");
            return NULL;
    }

    if (!keys) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;
    wrapped->hl_value = (vdynamic*)keys;
    wrapped->is_rooted = false;
    return wrapped;
}

hlffi_value* hlffi_map_values(hlffi_vm* vm, hlffi_value* map) {
    if (!vm || !map || !map->hl_value) return NULL;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;
    varray* values = NULL;

    switch (wrapper->type) {
        case HLFFI_MAP_INT:
            values = hl_intmap_values(wrapper->map_ptr);
            break;
        case HLFFI_MAP_STRING:
            values = hl_strbytesmap_values(wrapper->map_ptr);
            break;
        default:
            set_error(vm, HLFFI_ERROR_NOT_IMPLEMENTED, "Map type not supported");
            return NULL;
    }

    if (!values) return NULL;

    hlffi_value* wrapped = (hlffi_value*)malloc(sizeof(hlffi_value));
    if (!wrapped) return NULL;
    wrapped->hl_value = (vdynamic*)values;
    wrapped->is_rooted = false;
    return wrapped;
}

int hlffi_map_size(hlffi_value* map) {
    if (!map || !map->hl_value) return -1;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;

    switch (wrapper->type) {
        case HLFFI_MAP_INT:
            return hl_intmap_size(wrapper->map_ptr);
        case HLFFI_MAP_STRING:
            return hl_strbytesmap_size(wrapper->map_ptr);
        default:
            return -1;
    }
}

bool hlffi_map_clear(hlffi_value* map) {
    if (!map || !map->hl_value) return false;

    hlffi_map_wrapper* wrapper = (hlffi_map_wrapper*)map->hl_value;

    switch (wrapper->type) {
        case HLFFI_MAP_INT:
            hl_intmap_clear(wrapper->map_ptr);
            return true;
        case HLFFI_MAP_STRING:
            hl_strbytesmap_clear(wrapper->map_ptr);
            return true;
        default:
            return false;
    }
}
