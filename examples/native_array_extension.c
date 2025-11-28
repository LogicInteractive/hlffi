/**
 * Example Native Extension: Array Operations
 *
 * Demonstrates how to use hlffi_array_helpers.h in a real HashLink native extension.
 *
 * Compile:
 *   gcc -shared -fPIC -o array_ops.hdll native_array_extension.c \
 *       -I/path/to/hashlink/src -L/path/to/hashlink -lhl
 *
 * Haxe Usage:
 *   @:hlNative("array_ops", "sum_int_array")
 *   static function sumIntArray(arr:Array<Int>):Int;
 */

#include <hl.h>
#include "hlffi_array_helpers.h"

/* ========== Example 1: Simple Array Processing ========== */

/**
 * Sum all integers in an array
 * Haxe: function sumIntArray(arr:Array<Int>):Int
 */
HL_PRIM int HL_NAME(sum_int_array)(vdynamic* haxe_array) {
    int* data;
    int size;

    /* Fast path: direct memory access */
    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return 0;  /* Not a valid int array */
    }

    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum;
}
DEFINE_PRIM(_I32, sum_int_array, _DYN);

/* ========== Example 2: Array Creation ========== */

/**
 * Generate an array of squares: [0, 1, 4, 9, 16, ...]
 * Haxe: function generateSquares(count:Int):Array<Int>
 */
HL_PRIM vdynamic* HL_NAME(generate_squares)(int count) {
    if (count < 0) count = 0;

    /* Allocate temporary C array */
    int* values = (int*)malloc(count * sizeof(int));
    for (int i = 0; i < count; i++) {
        values[i] = i * i;
    }

    /* Convert to Haxe array */
    hl_code* code = HLFFI_GET_CODE();
    vdynamic* result = create_haxe_int_array(code, values, count);

    free(values);
    return result;
}
DEFINE_PRIM(_DYN, generate_squares, _I32);

/* ========== Example 3: In-Place Modification ========== */

/**
 * Double all values in an int array (modifies in place)
 * Haxe: function doubleValues(arr:Array<Int>):Void
 */
HL_PRIM void HL_NAME(double_values)(vdynamic* haxe_array) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return;
    }

    /* Modify in place - changes visible to Haxe immediately */
    for (int i = 0; i < size; i++) {
        data[i] *= 2;
    }
}
DEFINE_PRIM(_VOID, double_values, _DYN);

/* ========== Example 4: String Array Processing ========== */

/**
 * Concatenate all strings in an array with a separator
 * Haxe: function joinStrings(arr:Array<String>, sep:String):String
 */
HL_PRIM vbyte* HL_NAME(join_strings)(vdynamic* haxe_array, vbyte* separator) {
    varray* arr;
    if (!haxe_array_to_varray(haxe_array, &arr)) {
        return (vbyte*)USTR("");
    }

    if (arr->size == 0) {
        return (vbyte*)USTR("");
    }

    vbyte** strings = hl_aptr(arr, vbyte*);
    uchar* sep = (uchar*)separator;

    /* Calculate total length needed */
    int total_len = 0;
    for (int i = 0; i < arr->size; i++) {
        total_len += (int)ustrlen((uchar*)strings[i]);
        if (i < arr->size - 1) {
            total_len += (int)ustrlen(sep);
        }
    }

    /* Allocate result buffer (UTF-16) */
    uchar* result = (uchar*)malloc((total_len + 1) * sizeof(uchar));
    result[0] = 0;

    /* Concatenate strings */
    for (int i = 0; i < arr->size; i++) {
        ustrcat(result, (uchar*)strings[i]);
        if (i < arr->size - 1) {
            ustrcat(result, sep);
        }
    }

    return (vbyte*)result;
}
DEFINE_PRIM(_BYTES, join_strings, _DYN _BYTES);

/* ========== Example 5: Float Array Operations ========== */

/**
 * Calculate average of a float array
 * Haxe: function averageFloats(arr:Array<Float>):Float
 */
HL_PRIM double HL_NAME(average_floats)(vdynamic* haxe_array) {
    double* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size) || size == 0) {
        return 0.0;
    }

    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum / size;
}
DEFINE_PRIM(_F64, average_floats, _DYN);

/* ========== Example 6: Type-Specific F32 (Single) ========== */

/**
 * Normalize a Single (F32) array to range [0, 1]
 * Haxe: function normalizeSingles(arr:Array<Single>):Array<Single>
 *
 * IMPORTANT: This demonstrates F32 vs F64 distinction!
 */
HL_PRIM vdynamic* HL_NAME(normalize_singles)(vdynamic* haxe_array) {
    float* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size) || size == 0) {
        return haxe_array;
    }

    /* Find min and max */
    float min_val = data[0];
    float max_val = data[0];
    for (int i = 1; i < size; i++) {
        if (data[i] < min_val) min_val = data[i];
        if (data[i] > max_val) max_val = data[i];
    }

    float range = max_val - min_val;
    if (range == 0.0f) {
        return haxe_array;  /* All values same */
    }

    /* Create normalized array */
    float* normalized = (float*)malloc(size * sizeof(float));
    for (int i = 0; i < size; i++) {
        normalized[i] = (data[i] - min_val) / range;
    }

    hl_code* code = HLFFI_GET_CODE();
    vdynamic* result = create_haxe_single_array(code, normalized, size);

    free(normalized);
    return result;
}
DEFINE_PRIM(_DYN, normalize_singles, _DYN);

/* ========== Example 7: Mixed Type Array (Dynamic) ========== */

/**
 * Count non-null elements in a dynamic array
 * Haxe: function countNonNull(arr:Array<Dynamic>):Int
 */
HL_PRIM int HL_NAME(count_non_null)(vdynamic* haxe_array) {
    varray* arr;
    if (!haxe_array_to_varray(haxe_array, &arr)) {
        return 0;
    }

    vdynamic** data = hl_aptr(arr, vdynamic*);
    int count = 0;

    for (int i = 0; i < arr->size; i++) {
        if (data[i] != NULL) {
            count++;
        }
    }

    return count;
}
DEFINE_PRIM(_I32, count_non_null, _DYN);

/* ========== Example 8: Array Filtering ========== */

/**
 * Filter an int array, keeping only values > threshold
 * Haxe: function filterGreaterThan(arr:Array<Int>, threshold:Int):Array<Int>
 */
HL_PRIM vdynamic* HL_NAME(filter_greater_than)(vdynamic* haxe_array, int threshold) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return NULL;
    }

    /* First pass: count matches */
    int count = 0;
    for (int i = 0; i < size; i++) {
        if (data[i] > threshold) count++;
    }

    /* Second pass: copy matches */
    int* filtered = (int*)malloc(count * sizeof(int));
    int idx = 0;
    for (int i = 0; i < size; i++) {
        if (data[i] > threshold) {
            filtered[idx++] = data[i];
        }
    }

    hl_code* code = HLFFI_GET_CODE();
    vdynamic* result = create_haxe_int_array(code, filtered, count);

    free(filtered);
    return result;
}
DEFINE_PRIM(_DYN, filter_greater_than, _DYN _I32);

/* ========== Example 9: String Array Transformation ========== */

/**
 * Convert all strings to uppercase
 * Haxe: function toUpperCase(arr:Array<String>):Array<String>
 */
HL_PRIM vdynamic* HL_NAME(to_upper_case)(vdynamic* haxe_array) {
    varray* arr;
    if (!haxe_array_to_varray(haxe_array, &arr)) {
        return NULL;
    }

    vbyte** strings = hl_aptr(arr, vbyte*);

    /* Create new array with uppercase strings */
    varray* result_arr = hl_alloc_array(&hlt_bytes, arr->size);
    vbyte** result_data = hl_aptr(result_arr, vbyte*);

    for (int i = 0; i < arr->size; i++) {
        uchar* str = (uchar*)strings[i];
        int len = (int)ustrlen(str);

        /* Allocate new string */
        uchar* upper = (uchar*)malloc((len + 1) * sizeof(uchar));

        /* Convert to uppercase (simple ASCII version) */
        for (int j = 0; j < len; j++) {
            uchar c = str[j];
            if (c >= 'a' && c <= 'z') {
                upper[j] = c - 32;
            } else {
                upper[j] = c;
            }
        }
        upper[len] = 0;

        result_data[i] = (vbyte*)upper;
    }

    hl_code* code = HLFFI_GET_CODE();
    return varray_to_haxe_array(code, result_arr);
}
DEFINE_PRIM(_DYN, to_upper_case, _DYN);

/* ========== Example 10: Performance - Batch Operations ========== */

/**
 * Add a constant to all values (SIMD-friendly pattern)
 * Haxe: function addConstant(arr:Array<Int>, value:Int):Void
 */
HL_PRIM void HL_NAME(add_constant)(vdynamic* haxe_array, int value) {
    int* data;
    int size;

    if (!haxe_array_get_bytes(haxe_array, (void**)&data, &size)) {
        return;
    }

    /* Direct memory access - compiler can optimize/vectorize this */
    for (int i = 0; i < size; i++) {
        data[i] += value;
    }
}
DEFINE_PRIM(_VOID, add_constant, _DYN _I32);
