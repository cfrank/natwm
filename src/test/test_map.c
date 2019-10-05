// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>
#include <core/map.h>

static int test_setup(void **state)
{
        struct map *map = map_init();

        if (map == NULL) {
                return EXIT_FAILURE;
        }

        *state = map;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        map_destroy(*(struct map **)state);

        return EXIT_SUCCESS;
}

static void test_map_init(void **state)
{
        struct map *map = *(struct map **)state;

        assert_int_equal(MAP_MIN_LENGTH, map->length);
        assert_int_equal(0, map->bucket_count);
        assert_non_null(map->entries);
        assert_true(map->setting_flags & MAP_FLAG_IGNORE_THRESHOLDS_EMPTY);
        assert_true(map->event_flags & EVENT_FLAG_NORMAL);
}

static void test_map_insert(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "test_key";
        uint32_t expected_value = 14;
        enum map_error test_error
                = map_insert(map, expected_key, &expected_value);

        assert_int_equal(NO_ERROR, test_error);

        uint32_t result = map_get_uint32(map, expected_key, &test_error);

        assert_int_equal(NO_ERROR, test_error);
        assert_int_equal(expected_value, result);
}

static void test_map_insert_load_factor(void **state)
{
        struct map *map = *(struct map **)state;
        size_t value = 1;
        uint32_t expected_count = 3;
        uint32_t expected_length = MAP_MIN_LENGTH * 2;

        assert_int_equal(MAP_MIN_LENGTH, map->length);

        map_insert(map, "Hello", &value);
        map_insert(map, "Hello One", &value);
        // This will trigger a resize since the load factor is now
        // MAP_LOAD_FACTOR_HIGH
        map_insert(map, "Hello Two", &value);

        assert_int_equal(expected_count, map->bucket_count);
        assert_int_equal(expected_length, map->length);
}

static void test_map_insert_duplicate(void **state)
{
        struct map *map = *(struct map **)state;
        const char *first_value = "John Doe";
        const char *second_value = "Jane Doe";
        uint32_t expected_count = 1;

        map_insert(map, "name", &first_value);
        map_insert(map, "name", &second_value);

        assert_int_equal(expected_count, map->bucket_count);
}

static void test_map_get(void **state)
{
        struct map *map = *(struct map **)state;
        const char *key = "test";
        uint32_t value = 14;

        map_insert(map, key, &value);

        struct map_entry *result = map_get(map, key);

        assert_non_null(result);
        assert_string_equal(key, result->key);
        assert_ptr_equal(&value, result->value);
        assert_int_equal(value, *(uint32_t *)result->value);
}

static void test_map_get_duplicate(void **state)
{
        struct map *map = *(struct map **)state;
        const char *key = "test";
        char *const val_arr[] = {"First", "Second", NULL};
        struct map_entry *result = NULL;

        map_insert(map, key, val_arr[0]);

        result = map_get(map, key);

        assert_non_null(result);
        assert_string_equal(key, result->key);
        assert_string_equal(val_arr[0], result->value);

        map_insert(map, key, val_arr[1]);

        result = map_get(map, key);

        assert_non_null(result);
        assert_string_equal(key, result->key);
        assert_string_equal(val_arr[1], result->value);
}

static void test_map_delete(void **state)
{
        struct map *map = *(struct map **)state;
        const char *key = "test";
        uint32_t value = 14;
        enum map_error err = GENERIC_ERROR;

        map_insert(map, key, &value);

        uint32_t result = map_get_uint32(map, key, &err);

        assert_int_equal(NO_ERROR, err);
        assert_int_equal(value, result);

        assert_int_equal(NO_ERROR, map_delete(map, key));

        assert_null(map_get(map, key));
}

static void test_map_delete_resize(void **state)
{
        struct map *map = *(struct map **)state;
        const char *key = "testKey2";

        map_insert(map, "testKey3", "testValue");
        map_insert(map, key, "testValue");
        // This will trigger resize
        map_insert(map, "testKey5", "testValue");

        assert_int_equal(NO_ERROR, map_delete(map, key));
        assert_int_equal(8, map->length);
        assert_null(map_get(map, key));
}

static void test_map_delete_duplicate(void **state)
{
        struct map *map = *(struct map **)state;
        const char *key = "testKey2";
        uint32_t expected_value = 23;
        enum map_error err = GENERIC_ERROR;

        map_insert(map, key, "Value");
        map_insert(map, key, &expected_value);

        uint32_t result = map_get_uint32(map, key, &err);

        assert_int_equal(NO_ERROR, err);
        assert_int_equal(expected_value, result);

        assert_int_equal(NO_ERROR, map_delete(map, key));

        assert_int_equal(0, map->bucket_count);
        assert_null(map_get(map, key));
}

static void test_map_get_and_delete(void **state)
{
        struct map *map = *(struct map **)state;
        struct map_entry *result = NULL;

        // This should cause at least two resizes
        map_insert(map, "one", "1");
        map_insert(map, "two", "2");
        map_insert(map, "three", "3");
        map_insert(map, "four", "4");
        map_insert(map, "five", "5");
        map_insert(map, "six", "6");
        map_insert(map, "seven", "7");
        map_insert(map, "eight", "8");
        map_insert(map, "nine", "9");
        map_insert(map, "ten", "10");
        map_insert(map, "eleven", "11");
        map_insert(map, "twelve", "12");
        map_insert(map, "thirteen", "13");
        map_insert(map, "fourteen", "14");

        result = map_get(map, "six");

        assert_non_null(result);
        assert_string_equal("six", result->key);
        assert_string_equal("6", result->value);

        assert_int_equal(NO_ERROR, map_delete(map, "six"));

        result = map_get(map, "six");

        assert_null(result);

        result = map_get(map, "five");

        assert_non_null(result);
        assert_string_equal("five", result->key);
        assert_string_equal("5", result->value);

        result = map_get(map, "seven");

        assert_non_null(result);
        assert_string_equal("seven", result->key);
        assert_string_equal("7", result->value);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_map_init, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_load_factor, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_duplicate, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get_duplicate, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete_resize, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete_duplicate, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get_and_delete, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
