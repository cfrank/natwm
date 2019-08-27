// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cmocka.h>
#include <core/map.h>

static int test_setup(void **state)
{
        struct dict_map *map = map_init();

        if (map == NULL) {
                return EXIT_FAILURE;
        }

        *state = map;

        return EXIT_SUCCESS;
}

static int test_teardown(void **state)
{
        map_destroy(*(struct dict_map **)state);

        return EXIT_SUCCESS;
}

static size_t count_entries(const struct dict_map *map)
{
        size_t count = 0;

        for (size_t i = 0; i < map->length; ++i) {
                assert(i < map->length);

                struct dict_entry *entry = map->entries[i];

                if (entry != NULL && entry->key != NULL
                    && entry->data != NULL) {
                        count++;
                }
        }

        return count;
}

static void test(void **state)
{
        struct dict_map *map = *(struct dict_map **)state;

        assert_int_equal(MAP_MIN_LENGTH, map->length);
        assert_int_equal(0, count_entries(map));
}

int main(void)
{
        const struct CMUnitTest tests[] = {
                cmocka_unit_test_setup_teardown(
                        test, test_setup, test_teardown),
        };

        return cmocka_run_group_tests(tests, NULL, NULL);
}
