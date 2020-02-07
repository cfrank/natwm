// Copyright 2019 Chris Fraexpected_key// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>

#include <common/constants.h>
#include <common/error.h>
#include <common/map.h>

/*
 * Static functions for use in tests
 */
struct test_value {
        size_t length;
        uint32_t *test_values;
};

static struct test_value *test_value_init(void)
{
        struct test_value *value = malloc(sizeof(struct test_value));

        if (value == NULL) {
                return NULL;
        }

        value->length = 3;
        value->test_values = malloc(sizeof(uint32_t) * value->length);

        if (value->test_values == NULL) {
                free(value);

                return NULL;
        }

        for (size_t i = 0; i < value->length; ++i) {
                value->test_values[i] = (uint32_t)i;
        }

        return value;
}

static void test_value_destroy(void *data)
{
        struct test_value *value = (struct test_value *)data;

        if (value == NULL) {
                return;
        }

        free(value->test_values);
        free(value);
}

static size_t determine_number_key_size(const void *key)
{
        UNUSED_FUNCTION_PARAM(key);

        return sizeof(size_t *);
}

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
        const char *expected_key = "test";
        uint32_t expected_value = 123;
        enum natwm_error err = GENERIC_ERROR;

        err = map_insert(map, expected_key, &expected_value);

        assert_int_equal(NO_ERROR, err);

        uint32_t result = map_get_uint32(map, expected_key, &err);

        assert_int_equal(NO_ERROR, err);
        assert_int_equal(expected_value, result);
}

static void test_map_insert_null_value(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "test";

        assert_int_equal(NO_ERROR, map_insert(map, expected_key, NULL));

        struct map_entry *result = map_get(map, expected_key);

        assert_non_null(result);
        assert_string_equal(expected_key, result->key);
        assert_null(result->value);
}

static void test_map_insert_null_key(void **state)
{
        struct map *map = *(struct map **)state;

        assert_int_equal(GENERIC_ERROR, map_insert(map, NULL, NULL));
}

static void test_map_insert_non_string_key(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        // We don't want to override the key sizing of the rest of the tests
        struct map *map = map_init();

        if (map == NULL) {
                assert_true(false);
        }

        map_set_key_size_function(map, determine_number_key_size);

        size_t expected_key = 12345;
        const char *expected_value = "value";

        assert_int_equal(
                NO_ERROR,
                map_insert(map, &expected_key, (char *)expected_value));

        struct map_entry *entry = map_get(map, &expected_key);

        assert_non_null(entry);
        assert_int_equal(expected_key, *(size_t *)entry->key);
        assert_string_equal(expected_value, (const char *)entry->value);
}

static void test_map_insert_multiple_non_string_key(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        struct map *map = map_init();

        if (map == NULL) {
                assert_true(false);
        }

        map_set_key_size_function(map, determine_number_key_size);

        size_t expected_key_first = 1234;
        size_t expected_key_second = 4321;
        const char *expected_value_first = "first value";
        const char *expected_value_second = "second value";
        size_t expected_bucket_count = 2;

        assert_int_equal(NO_ERROR,
                         map_insert(map,
                                    &expected_key_first,
                                    (char *)expected_value_first));
        assert_int_equal(NO_ERROR,
                         map_insert(map,
                                    &expected_key_second,
                                    (char *)expected_value_second));

        assert_int_equal(expected_bucket_count, map->bucket_count);

        struct map_entry *first_entry = map_get(map, &expected_key_first);
        struct map_entry *second_entry = map_get(map, &expected_key_second);

        assert_non_null(first_entry);
        assert_non_null(second_entry);
        assert_string_equal(expected_value_first,
                            (const char *)first_entry->value);
        assert_int_equal(expected_key_first, *(size_t *)first_entry->key);
        assert_string_equal(expected_value_second,
                            (const char *)second_entry->value);
        assert_int_equal(expected_key_second, *(size_t *)second_entry->key);
}

static void test_map_insert_load_factor_disabled(void **state)
{
        struct map *map = *(struct map **)state;

        map->setting_flags |= MAP_FLAG_IGNORE_THRESHOLDS;

        map_insert(map, "test1", "value");
        map_insert(map, "test2", "value");
        map_insert(map, "test3", "value");
        map_insert(map, "test4", "value");

        assert_int_equal(MAP_MIN_LENGTH, map->length);
        assert_int_equal(4, map->bucket_count);

        assert_int_equal(CAPACITY_ERROR, map_insert(map, "test5", "value"));
}

static void test_map_insert_load_factor(void **state)
{
        struct map *map = *(struct map **)state;
        uint32_t expected_count = 3;
        uint32_t expected_length = MAP_MIN_LENGTH * 2;

        assert_int_equal(MAP_MIN_LENGTH, map->length);

        map_insert(map, "test1", "value");
        map_insert(map, "test2", "value");

        assert_int_equal(MAP_MIN_LENGTH, map->length);

        // This will trigger a resize since the load factor is now
        // MAP_LOAD_FACTOR_HIGH
        map_insert(map, "test3", "value");

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
        const char *expected_key = "test";
        uint32_t expected_value = 123;

        map_insert(map, expected_key, &expected_value);

        struct map_entry *result = map_get(map, expected_key);

        assert_non_null(result);
        assert_string_equal(expected_key, result->key);
        assert_ptr_equal(&expected_value, result->value);
        assert_int_equal(expected_value, *(uint32_t *)result->value);
}

static void test_map_get_empty(void **state)
{
        struct map *map = *(struct map **)state;

        map_insert(map, "test", "value");

        struct map_entry *result = map_get(map, "unknown");

        assert_null(result);
}

static void test_map_get_null(void **state)
{
        struct map *map = *(struct map **)state;
        struct map_entry *result = map_get(map, NULL);

        assert_null(result);
}

static void test_map_get_duplicate(void **state)
{
        struct map *map = *(struct map **)state;
        struct map_entry *result = NULL;
        const char *expected_key = "test";
        char *const expected_values[2] = {"first", "second"};

        map_insert(map, expected_key, expected_values[0]);

        result = map_get(map, expected_key);

        assert_non_null(result);
        assert_string_equal(expected_key, result->key);
        assert_string_equal(expected_values[0], result->value);

        map_insert(map, expected_key, expected_values[1]);

        result = map_get(map, expected_key);

        assert_non_null(result);
        assert_string_equal(expected_key, result->key);
        assert_string_equal(expected_values[1], result->value);
}

static void test_map_delete(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "test";
        uint32_t expected_value = 123;
        enum natwm_error err = GENERIC_ERROR;

        map_insert(map, expected_key, &expected_value);

        uint32_t result = map_get_uint32(map, expected_key, &err);

        assert_int_equal(NO_ERROR, err);
        assert_int_equal(expected_value, result);

        assert_int_equal(NO_ERROR, map_delete(map, expected_key));

        assert_null(map_get(map, expected_key));
}

static void test_map_delete_null(void **state)
{
        struct map *map = *(struct map **)state;

        assert_int_equal(GENERIC_ERROR, map_delete(map, NULL));
}

static void test_map_delete_unknown(void **state)
{
        struct map *map = *(struct map **)state;

        map_insert(map, "test", "value");

        assert_int_equal(NOT_FOUND_ERROR, map_delete(map, "unknown"));
}

static void test_map_delete_duplicate(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "testKey2";
        uint32_t expected_value = 123;
        enum natwm_error err = GENERIC_ERROR;

        map_insert(map, expected_key, "value");
        map_insert(map, expected_key, &expected_value);

        uint32_t result = map_get_uint32(map, expected_key, &err);

        assert_int_equal(NO_ERROR, err);
        assert_int_equal(expected_value, result);

        assert_int_equal(NO_ERROR, map_delete(map, expected_key));

        assert_int_equal(0, map->bucket_count);
        assert_null(map_get(map, expected_key));
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

static void test_map_destroy_null(void **state)
{
        UNUSED_FUNCTION_PARAM(state);

        map_destroy(NULL);
}

static void test_map_destroy_use_free(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "test";
        size_t value_length = 5;
        size_t *allocated_value = malloc(sizeof(size_t) * value_length);

        assert_non_null(allocated_value);

        for (size_t i = 0; i < value_length; ++i) {
                allocated_value[i] = i;
        }

        map->setting_flags |= MAP_FLAG_USE_FREE;

        map_insert(map, expected_key, allocated_value);
}

static void test_map_destroy_free_func(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "test";
        struct test_value *allocated_value = test_value_init();

        assert_non_null(allocated_value);

        map_set_entry_free_function(map, test_value_destroy);

        map_insert(map, expected_key, allocated_value);
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test_map_init, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_null_value, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_null_key, test_setup, test_teardown),
                cmocka_unit_test(test_map_insert_non_string_key),
                cmocka_unit_test(test_map_insert_multiple_non_string_key),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_load_factor_disabled,
                        test_setup,
                        test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_load_factor, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_insert_duplicate, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get_empty, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get_null, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get_duplicate, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete_null, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete_unknown, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_delete_duplicate, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_get_and_delete, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_destroy_null, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_destroy_use_free, test_setup, test_teardown),
                cmocka_unit_test_setup_teardown(
                        test_map_destroy_free_func, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
