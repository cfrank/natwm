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

static size_t count_entries(const struct map *map)
{
        size_t count = 0;

        for (size_t i = 0; i < map->length; ++i) {
                struct map_entry *entry = map->entries[i];

                if (entry != NULL && entry->key != NULL
                    && entry->value != NULL) {
                        count++;
                }
        }

        return count;
}

static void test_map_init(void **state)
{
        struct map *map = *(struct map **)state;

        assert_int_equal(MAP_MIN_LENGTH, map->length);
        assert_int_equal(0, map->bucket_count);
        assert_non_null(map->entries);
        assert_int_equal(0, count_entries(map));
        assert_true(map->setting_flags & MAP_FLAG_IGNORE_THRESHOLDS_EMPTY);
        assert_true(map->event_flags & EVENT_FLAG_NORMAL);
}

static void test_map_insert(void **state)
{
        struct map *map = *(struct map **)state;
        const char *expected_key = "test_key";
        size_t expected_value = 14;
        enum map_error error = map_insert(map, expected_key, &expected_value);

        assert_int_equal(NO_ERROR, error);
        assert_int_equal(1, count_entries(map));
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
        assert_int_equal(expected_count, count_entries(map));
        assert_int_equal(expected_length, map->length);
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
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
