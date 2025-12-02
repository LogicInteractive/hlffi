/**
 * Phase 3 Extended Test: Static Fields and Methods
 * Comprehensive tests for value boxing/unboxing, static field access, and static method calls
 * Tests edge cases: negative numbers, zero, large values, unicode strings, booleans, etc.
 */

#include "hlffi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) printf("--- %s ---\n", name)
#define TEST_PASS(msg) do { printf("  PASS: %s\n", msg); tests_passed++; } while(0)
#define TEST_FAIL(msg) do { printf("  FAIL: %s\n", msg); tests_failed++; } while(0)

#define ASSERT_INT_EQ(actual, expected, msg) do { \
    if ((actual) == (expected)) { TEST_PASS(msg); } \
    else { printf("  FAIL: %s (expected %d, got %d)\n", msg, expected, actual); tests_failed++; } \
} while(0)

#define ASSERT_FLOAT_EQ(actual, expected, msg) do { \
    if (fabs((actual) - (expected)) < 0.0001) { TEST_PASS(msg); } \
    else { printf("  FAIL: %s (expected %.6f, got %.6f)\n", msg, expected, actual); tests_failed++; } \
} while(0)

#define ASSERT_BOOL_EQ(actual, expected, msg) do { \
    if ((actual) == (expected)) { TEST_PASS(msg); } \
    else { printf("  FAIL: %s (expected %s, got %s)\n", msg, expected ? "true" : "false", actual ? "true" : "false"); tests_failed++; } \
} while(0)

#define ASSERT_STR_EQ(actual, expected, msg) do { \
    if ((actual) && (expected) && strcmp((actual), (expected)) == 0) { TEST_PASS(msg); } \
    else { printf("  FAIL: %s (expected \"%s\", got \"%s\")\n", msg, expected ? expected : "(null)", actual ? actual : "(null)"); tests_failed++; } \
} while(0)

#define ASSERT_NOT_NULL(ptr, msg) do { \
    if ((ptr) != NULL) { TEST_PASS(msg); } \
    else { TEST_FAIL(msg); } \
} while(0)

// Helper to call static method with no args and get int result
static int call_get_int(hlffi_vm* vm, const char* class_name, const char* method) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method, 0, NULL);
    int val = result ? hlffi_value_as_int(result, -99999) : -99999;
    if (result) free(result);
    return val;
}

// Helper to call static method with no args and get float result
static double call_get_float(hlffi_vm* vm, const char* class_name, const char* method) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method, 0, NULL);
    double val = result ? hlffi_value_as_float(result, -99999.0) : -99999.0;
    if (result) free(result);
    return val;
}

// Helper to call static method with no args and get bool result
static bool call_get_bool(hlffi_vm* vm, const char* class_name, const char* method) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method, 0, NULL);
    bool val = result ? hlffi_value_as_bool(result, false) : false;
    if (result) free(result);
    return val;
}

// Helper to call static method with no args and get string result (caller must free)
static char* call_get_string(hlffi_vm* vm, const char* class_name, const char* method) {
    hlffi_value* result = hlffi_call_static(vm, class_name, method, 0, NULL);
    char* val = result ? hlffi_value_as_string(result) : NULL;
    if (result) free(result);
    return val;
}

void test_static_field_edge_cases(hlffi_vm* vm) {
    TEST_START("Static Field Edge Cases");

    hlffi_value* val;
    char* str;

    // Negative integer
    printf("  Testing negativeInt...\n");
    val = hlffi_get_static_field(vm, "Game", "negativeInt");
    if (val) {
        ASSERT_INT_EQ(hlffi_value_as_int(val, 0), -42, "negativeInt = -42");
        free(val);
    } else {
        printf("  SKIP: negativeInt field not found: %s\n", hlffi_get_error(vm));
    }

    // Zero integer
    printf("  Testing zeroInt...\n");
    val = hlffi_get_static_field(vm, "Game", "zeroInt");
    if (val) {
        ASSERT_INT_EQ(hlffi_value_as_int(val, -1), 0, "zeroInt = 0");
        free(val);
    } else {
        printf("  SKIP: zeroInt field not found: %s\n", hlffi_get_error(vm));
    }

    // Large integer (max int32)
    printf("  Testing largeInt...\n");
    val = hlffi_get_static_field(vm, "Game", "largeInt");
    if (val) {
        ASSERT_INT_EQ(hlffi_value_as_int(val, 0), 2147483647, "largeInt = 2147483647");
        free(val);
    } else {
        printf("  SKIP: largeInt field not found: %s\n", hlffi_get_error(vm));
    }

    // Empty string
    printf("  Testing emptyString...\n");
    val = hlffi_get_static_field(vm, "Game", "emptyString");
    if (val) {
        str = hlffi_value_as_string(val);
        ASSERT_STR_EQ(str, "", "emptyString = \"\"");
        if (str) free(str);
        free(val);
    } else {
        printf("  SKIP: emptyString field not found: %s\n", hlffi_get_error(vm));
    }

    // Zero float
    printf("  Testing zeroFloat...\n");
    val = hlffi_get_static_field(vm, "Game", "zeroFloat");
    if (val) {
        ASSERT_FLOAT_EQ(hlffi_value_as_float(val, -1.0), 0.0, "zeroFloat = 0.0");
        free(val);
    } else {
        printf("  SKIP: zeroFloat field not found: %s\n", hlffi_get_error(vm));
    }

    // Negative float
    printf("  Testing negativeFloat...\n");
    val = hlffi_get_static_field(vm, "Game", "negativeFloat");
    if (val) {
        ASSERT_FLOAT_EQ(hlffi_value_as_float(val, 0.0), -3.14159, "negativeFloat = -3.14159");
        free(val);
    } else {
        printf("  SKIP: negativeFloat field not found: %s\n", hlffi_get_error(vm));
    }

    // False boolean
    printf("  Testing falseValue...\n");
    val = hlffi_get_static_field(vm, "Game", "falseValue");
    if (val) {
        ASSERT_BOOL_EQ(hlffi_value_as_bool(val, true), false, "falseValue = false");
        free(val);
    } else {
        printf("  SKIP: falseValue field not found: %s\n", hlffi_get_error(vm));
    }

    printf("\n");
}

void test_boolean_operations(hlffi_vm* vm) {
    TEST_START("Boolean Operations");

    // Get current state
    bool active = call_get_bool(vm, "Game", "isActive");
    printf("  Initial isActive: %s\n", active ? "true" : "false");

    // Toggle
    hlffi_value* result = hlffi_call_static(vm, "Game", "toggleRunning", 0, NULL);
    bool toggled = hlffi_value_as_bool(result, active);
    free(result);
    ASSERT_BOOL_EQ(toggled, !active, "toggleRunning inverts value");

    // AND operation
    hlffi_value* t = hlffi_value_bool(vm, true);
    hlffi_value* f = hlffi_value_bool(vm, false);
    hlffi_value* args[2];

    args[0] = t; args[1] = t;
    result = hlffi_call_static(vm, "Game", "andBool", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, false), true, "true AND true = true");
    free(result);

    args[0] = t; args[1] = f;
    result = hlffi_call_static(vm, "Game", "andBool", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, true), false, "true AND false = false");
    free(result);

    // OR operation
    args[0] = f; args[1] = f;
    result = hlffi_call_static(vm, "Game", "orBool", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, true), false, "false OR false = false");
    free(result);

    args[0] = f; args[1] = t;
    result = hlffi_call_static(vm, "Game", "orBool", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, false), true, "false OR true = true");
    free(result);

    // NOT operation
    hlffi_value* args1[1] = {t};
    result = hlffi_call_static(vm, "Game", "notBool", 1, args1);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, true), false, "NOT true = false");
    free(result);

    args1[0] = f;
    result = hlffi_call_static(vm, "Game", "notBool", 1, args1);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, false), true, "NOT false = true");
    free(result);

    free(t);
    free(f);

    printf("\n");
}

void test_integer_operations(hlffi_vm* vm) {
    TEST_START("Integer Operations");

    hlffi_value* result;
    hlffi_value* args[2];

    // Negate
    hlffi_value* n42 = hlffi_value_int(vm, 42);
    hlffi_value* args1[1] = {n42};
    result = hlffi_call_static(vm, "Game", "negate", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), -42, "negate(42) = -42");
    free(result);
    free(n42);

    // Negate negative
    hlffi_value* nm100 = hlffi_value_int(vm, -100);
    args1[0] = nm100;
    result = hlffi_call_static(vm, "Game", "negate", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 100, "negate(-100) = 100");
    free(result);
    free(nm100);

    // Subtract
    hlffi_value* a = hlffi_value_int(vm, 100);
    hlffi_value* b = hlffi_value_int(vm, 37);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "subtract", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 63, "subtract(100, 37) = 63");
    free(result);
    free(a); free(b);

    // Division
    a = hlffi_value_int(vm, 17);
    b = hlffi_value_int(vm, 5);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "divideInt", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 3, "divideInt(17, 5) = 3");
    free(result);
    free(a); free(b);

    // Modulo
    a = hlffi_value_int(vm, 17);
    b = hlffi_value_int(vm, 5);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "modulo", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 2, "modulo(17, 5) = 2");
    free(result);
    free(a); free(b);

    // Absolute value of negative
    hlffi_value* neg = hlffi_value_int(vm, -999);
    args1[0] = neg;
    result = hlffi_call_static(vm, "Game", "absInt", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 999, "absInt(-999) = 999");
    free(result);
    free(neg);

    // Max
    a = hlffi_value_int(vm, 42);
    b = hlffi_value_int(vm, 99);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "maxInt", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 99, "maxInt(42, 99) = 99");
    free(result);
    free(a); free(b);

    // Min
    a = hlffi_value_int(vm, 42);
    b = hlffi_value_int(vm, 99);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "minInt", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), 42, "minInt(42, 99) = 42");
    free(result);
    free(a); free(b);

    printf("\n");
}

void test_float_operations(hlffi_vm* vm) {
    TEST_START("Float Operations");

    hlffi_value* result;
    hlffi_value* args[2];

    // Division
    hlffi_value* a = hlffi_value_float(vm, 10.0);
    hlffi_value* b = hlffi_value_float(vm, 4.0);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "divide", 2, args);
    ASSERT_FLOAT_EQ(hlffi_value_as_float(result, 0.0), 2.5, "divide(10.0, 4.0) = 2.5");
    free(result);
    free(a); free(b);

    // Square root
    hlffi_value* n16 = hlffi_value_float(vm, 16.0);
    hlffi_value* args1[1] = {n16};
    result = hlffi_call_static(vm, "Game", "sqrtFloat", 1, args1);
    ASSERT_FLOAT_EQ(hlffi_value_as_float(result, 0.0), 4.0, "sqrtFloat(16.0) = 4.0");
    free(result);
    free(n16);

    // Power
    a = hlffi_value_float(vm, 2.0);
    b = hlffi_value_float(vm, 10.0);
    args[0] = a; args[1] = b;
    result = hlffi_call_static(vm, "Game", "powFloat", 2, args);
    ASSERT_FLOAT_EQ(hlffi_value_as_float(result, 0.0), 1024.0, "powFloat(2.0, 10.0) = 1024.0");
    free(result);
    free(a); free(b);

    // Floor
    hlffi_value* n37 = hlffi_value_float(vm, 3.7);
    args1[0] = n37;
    result = hlffi_call_static(vm, "Game", "floorFloat", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 3, "floorFloat(3.7) = 3");
    free(result);
    free(n37);

    // Ceil
    hlffi_value* n32 = hlffi_value_float(vm, 3.2);
    args1[0] = n32;
    result = hlffi_call_static(vm, "Game", "ceilFloat", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 4, "ceilFloat(3.2) = 4");
    free(result);
    free(n32);

    // Round
    hlffi_value* n35 = hlffi_value_float(vm, 3.5);
    args1[0] = n35;
    result = hlffi_call_static(vm, "Game", "roundFloat", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 4, "roundFloat(3.5) = 4");
    free(result);
    free(n35);

    // Absolute value of negative float
    hlffi_value* negf = hlffi_value_float(vm, -123.456);
    args1[0] = negf;
    result = hlffi_call_static(vm, "Game", "absFloat", 1, args1);
    ASSERT_FLOAT_EQ(hlffi_value_as_float(result, 0.0), 123.456, "absFloat(-123.456) = 123.456");
    free(result);
    free(negf);

    printf("\n");
}

void test_string_operations(hlffi_vm* vm) {
    TEST_START("String Operations");

    hlffi_value* result;
    hlffi_value* args[3];
    char* str;

    // Concatenation
    hlffi_value* hello = hlffi_value_string(vm, "Hello, ");
    hlffi_value* world = hlffi_value_string(vm, "World!");
    args[0] = hello; args[1] = world;
    result = hlffi_call_static(vm, "Game", "concat", 2, args);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "Hello, World!", "concat(\"Hello, \", \"World!\") = \"Hello, World!\"");
    if (str) free(str);
    free(result);
    free(hello); free(world);

    // String length
    hlffi_value* test = hlffi_value_string(vm, "HLFFI");
    hlffi_value* args1[1] = {test};
    result = hlffi_call_static(vm, "Game", "stringLength", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 5, "stringLength(\"HLFFI\") = 5");
    free(result);
    free(test);

    // Empty string length
    hlffi_value* empty = hlffi_value_string(vm, "");
    args1[0] = empty;
    result = hlffi_call_static(vm, "Game", "stringLength", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 0, "stringLength(\"\") = 0");
    free(result);
    free(empty);

    // toUpper
    hlffi_value* lower = hlffi_value_string(vm, "hello");
    args1[0] = lower;
    result = hlffi_call_static(vm, "Game", "toUpper", 1, args1);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "HELLO", "toUpper(\"hello\") = \"HELLO\"");
    if (str) free(str);
    free(result);
    free(lower);

    // toLower
    hlffi_value* upper = hlffi_value_string(vm, "WORLD");
    args1[0] = upper;
    result = hlffi_call_static(vm, "Game", "toLower", 1, args1);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "world", "toLower(\"WORLD\") = \"world\"");
    if (str) free(str);
    free(result);
    free(upper);

    // Substring
    hlffi_value* src = hlffi_value_string(vm, "Hello, World!");
    hlffi_value* start = hlffi_value_int(vm, 7);
    hlffi_value* len = hlffi_value_int(vm, 5);
    args[0] = src; args[1] = start; args[2] = len;
    result = hlffi_call_static(vm, "Game", "substring", 3, args);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "World", "substring(\"Hello, World!\", 7, 5) = \"World\"");
    if (str) free(str);
    free(result);
    free(src); free(start); free(len);

    // Repeat string
    hlffi_value* ab = hlffi_value_string(vm, "ab");
    hlffi_value* count = hlffi_value_int(vm, 3);
    args[0] = ab; args[1] = count;
    result = hlffi_call_static(vm, "Game", "repeatString", 2, args);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "ababab", "repeatString(\"ab\", 3) = \"ababab\"");
    if (str) free(str);
    free(result);
    free(ab); free(count);

    // Reverse string
    hlffi_value* abc = hlffi_value_string(vm, "abc123");
    args1[0] = abc;
    result = hlffi_call_static(vm, "Game", "reverseString", 1, args1);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "321cba", "reverseString(\"abc123\") = \"321cba\"");
    if (str) free(str);
    free(result);
    free(abc);

    printf("\n");
}

void test_type_conversions(hlffi_vm* vm) {
    TEST_START("Type Conversions");

    hlffi_value* result;
    hlffi_value* args1[1];
    char* str;

    // Int to string
    hlffi_value* n42 = hlffi_value_int(vm, 42);
    args1[0] = n42;
    result = hlffi_call_static(vm, "Game", "intToString", 1, args1);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "42", "intToString(42) = \"42\"");
    if (str) free(str);
    free(result);
    free(n42);

    // Negative int to string
    hlffi_value* nm99 = hlffi_value_int(vm, -99);
    args1[0] = nm99;
    result = hlffi_call_static(vm, "Game", "intToString", 1, args1);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "-99", "intToString(-99) = \"-99\"");
    if (str) free(str);
    free(result);
    free(nm99);

    // String to int
    hlffi_value* s123 = hlffi_value_string(vm, "123");
    args1[0] = s123;
    result = hlffi_call_static(vm, "Game", "stringToInt", 1, args1);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 123, "stringToInt(\"123\") = 123");
    free(result);
    free(s123);

    // String to float
    hlffi_value* s3_14 = hlffi_value_string(vm, "3.14");
    args1[0] = s3_14;
    result = hlffi_call_static(vm, "Game", "stringToFloat", 1, args1);
    ASSERT_FLOAT_EQ(hlffi_value_as_float(result, 0.0), 3.14, "stringToFloat(\"3.14\") = 3.14");
    free(result);
    free(s3_14);

    printf("\n");
}

void test_multiple_args(hlffi_vm* vm) {
    TEST_START("Multiple Arguments (3+)");

    hlffi_value* result;
    hlffi_value* args[4];
    char* str;

    // Sum of 3 integers
    args[0] = hlffi_value_int(vm, 10);
    args[1] = hlffi_value_int(vm, 20);
    args[2] = hlffi_value_int(vm, 30);
    result = hlffi_call_static(vm, "Game", "sum3", 3, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 60, "sum3(10, 20, 30) = 60");
    free(result);
    free(args[0]); free(args[1]); free(args[2]);

    // Sum of 4 integers
    args[0] = hlffi_value_int(vm, 1);
    args[1] = hlffi_value_int(vm, 2);
    args[2] = hlffi_value_int(vm, 3);
    args[3] = hlffi_value_int(vm, 4);
    result = hlffi_call_static(vm, "Game", "sum4", 4, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 10, "sum4(1, 2, 3, 4) = 10");
    free(result);
    free(args[0]); free(args[1]); free(args[2]); free(args[3]);

    // Average of 3 floats
    args[0] = hlffi_value_float(vm, 3.0);
    args[1] = hlffi_value_float(vm, 6.0);
    args[2] = hlffi_value_float(vm, 9.0);
    result = hlffi_call_static(vm, "Game", "avg3", 3, args);
    ASSERT_FLOAT_EQ(hlffi_value_as_float(result, -1.0), 6.0, "avg3(3.0, 6.0, 9.0) = 6.0");
    free(result);
    free(args[0]); free(args[1]); free(args[2]);

    // Format score (string, int, int)
    args[0] = hlffi_value_string(vm, "Alice");
    args[1] = hlffi_value_int(vm, 1500);
    args[2] = hlffi_value_int(vm, 7);
    result = hlffi_call_static(vm, "Game", "formatScore", 3, args);
    str = hlffi_value_as_string(result);
    ASSERT_STR_EQ(str, "Alice: 1500 pts (Level 7)", "formatScore(\"Alice\", 1500, 7)");
    if (str) free(str);
    free(result);
    free(args[0]); free(args[1]); free(args[2]);

    printf("\n");
}

void test_comparison_operations(hlffi_vm* vm) {
    TEST_START("Comparison Operations");

    hlffi_value* result;
    hlffi_value* args[2];

    // isGreater
    args[0] = hlffi_value_int(vm, 10);
    args[1] = hlffi_value_int(vm, 5);
    result = hlffi_call_static(vm, "Game", "isGreater", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, false), true, "isGreater(10, 5) = true");
    free(result);
    free(args[0]); free(args[1]);

    args[0] = hlffi_value_int(vm, 5);
    args[1] = hlffi_value_int(vm, 10);
    result = hlffi_call_static(vm, "Game", "isGreater", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, true), false, "isGreater(5, 10) = false");
    free(result);
    free(args[0]); free(args[1]);

    // isEqual
    args[0] = hlffi_value_int(vm, 42);
    args[1] = hlffi_value_int(vm, 42);
    result = hlffi_call_static(vm, "Game", "isEqual", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, false), true, "isEqual(42, 42) = true");
    free(result);
    free(args[0]); free(args[1]);

    args[0] = hlffi_value_int(vm, 1);
    args[1] = hlffi_value_int(vm, 2);
    result = hlffi_call_static(vm, "Game", "isEqual", 2, args);
    ASSERT_BOOL_EQ(hlffi_value_as_bool(result, true), false, "isEqual(1, 2) = false");
    free(result);
    free(args[0]); free(args[1]);

    // compareStrings
    args[0] = hlffi_value_string(vm, "apple");
    args[1] = hlffi_value_string(vm, "banana");
    result = hlffi_call_static(vm, "Game", "compareStrings", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, 0), -1, "compareStrings(\"apple\", \"banana\") = -1");
    free(result);
    free(args[0]); free(args[1]);

    args[0] = hlffi_value_string(vm, "same");
    args[1] = hlffi_value_string(vm, "same");
    result = hlffi_call_static(vm, "Game", "compareStrings", 2, args);
    ASSERT_INT_EQ(hlffi_value_as_int(result, -1), 0, "compareStrings(\"same\", \"same\") = 0");
    free(result);
    free(args[0]); free(args[1]);

    printf("\n");
}

void test_void_returns(hlffi_vm* vm) {
    TEST_START("Void Returns");

    // doNothing should not crash
    hlffi_value* result = hlffi_call_static(vm, "Game", "doNothing", 0, NULL);
    ASSERT_NOT_NULL(result, "doNothing() returns (null wrapper)");
    free(result);

    // printMessage should work
    hlffi_value* msg = hlffi_value_string(vm, "Test message from C!");
    hlffi_value* args[1] = {msg};
    result = hlffi_call_static(vm, "Game", "printMessage", 1, args);
    ASSERT_NOT_NULL(result, "printMessage() returns (null wrapper)");
    free(result);
    free(msg);

    printf("\n");
}

void test_set_and_verify_fields(hlffi_vm* vm) {
    TEST_START("Set and Verify Fields");

    hlffi_value* val;
    char* str;

    // Set boolean field
    hlffi_value* new_bool = hlffi_value_bool(vm, false);
    hlffi_set_static_field(vm, "Game", "isRunning", new_bool);
    val = hlffi_get_static_field(vm, "Game", "isRunning");
    ASSERT_BOOL_EQ(hlffi_value_as_bool(val, true), false, "Set isRunning = false");
    free(val);
    free(new_bool);

    // Set float field
    hlffi_value* new_float = hlffi_value_float(vm, 9.99);
    hlffi_set_static_field(vm, "Game", "multiplier", new_float);
    val = hlffi_get_static_field(vm, "Game", "multiplier");
    ASSERT_FLOAT_EQ(hlffi_value_as_float(val, 0.0), 9.99, "Set multiplier = 9.99");
    free(val);
    free(new_float);

    // Set negative int
    hlffi_value* neg_int = hlffi_value_int(vm, -500);
    hlffi_set_static_field(vm, "Game", "score", neg_int);
    val = hlffi_get_static_field(vm, "Game", "score");
    ASSERT_INT_EQ(hlffi_value_as_int(val, 0), -500, "Set score = -500");
    free(val);
    free(neg_int);

    printf("\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <bytecode.hl>\n", argv[0]);
        return 1;
    }

    printf("=================================================================\n");
    printf("  Phase 3 Extended Test Suite: Static Members & Values\n");
    printf("=================================================================\n\n");

    // Create and initialize VM
    hlffi_vm* vm = hlffi_create();
    if (!vm) {
        fprintf(stderr, "ERROR: Failed to create VM\n");
        return 1;
    }

    hlffi_error_code err = hlffi_init(vm, 0, NULL);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Init failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    err = hlffi_load_file(vm, argv[1]);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Load failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }

    // CRITICAL: Call entry point to initialize static fields
    printf("Initializing VM and calling entry point...\n");
    fflush(stdout);
    err = hlffi_call_entry(vm);
    printf("DEBUG: hlffi_call_entry returned %d\n", err);
    fflush(stdout);
    if (err != HLFFI_OK) {
        fprintf(stderr, "ERROR: Entry point failed: %s\n", hlffi_get_error(vm));
        hlffi_destroy(vm);
        return 1;
    }
    printf("VM initialized successfully.\n\n");
    fflush(stdout);

    // Run all test suites
    printf("DEBUG: Starting test_static_field_edge_cases\n");
    fflush(stdout);
    test_static_field_edge_cases(vm);
    test_boolean_operations(vm);
    test_integer_operations(vm);
    test_float_operations(vm);
    test_string_operations(vm);
    test_type_conversions(vm);
    test_multiple_args(vm);
    test_comparison_operations(vm);
    test_void_returns(vm);
    test_set_and_verify_fields(vm);

    // Cleanup
    hlffi_destroy(vm);

    // Summary
    printf("=================================================================\n");
    printf("  TEST SUMMARY\n");
    printf("=================================================================\n");
    printf("  PASSED: %d\n", tests_passed);
    printf("  FAILED: %d\n", tests_failed);
    printf("  TOTAL:  %d\n", tests_passed + tests_failed);
    printf("=================================================================\n");

    if (tests_failed > 0) {
        printf("\n  *** SOME TESTS FAILED ***\n\n");
        return 1;
    } else {
        printf("\n  *** ALL TESTS PASSED ***\n\n");
        return 0;
    }
}
