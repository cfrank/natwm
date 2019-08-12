// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <common/hash.h>
#include <common/util.h>
#include "dict.h"

static ATTR_INLINE char *duplicate_key_lower(const char *key)
{
        char *new_key = alloc_string(key);

        if (new_key == NULL) {
                return NULL;
        }

        for (size_t i = 0; new_key[i]; ++i) {
                new_key[i] = (char)tolower(new_key[i]);
        }

        return new_key;
}

static ATTR_CONST ATTR_INLINE uint32_t get_power_two(uint32_t num)
{
        uint32_t i = 1;

        while (i < num) {
                i <<= 1;
        }

        return i;
}

static struct dict_table *create_map_internal(size_t size, uint8_t flags)
{
        struct dict_table *table = malloc(sizeof(struct dict_table));

        if (table == NULL) {
                return NULL;
        }

        table->bucket_count = get_power_two((uint32_t)size);
        table->entries_count = 0;
        table->entries = NULL;
#ifdef USE_POSIX
        if (pthread_mutex_init(&table->mutex, NULL) != 0) {
                return NULL;
        }
#endif
        table->flags = flags;
        table->hash_function = key_hash;
        table->itr_bucket_index = 0;
        table->high_load_factor = 0.75;
        table->low_load_factor = 0.20;
        table->free_function = NULL;
        table->event_flags = 0;

        return table;
}

static ATTR_INLINE uint32_t hash_value(const struct dict_table *table,
                                       const char *key)
{
        assert(key);

        if (table->flags & DICT_KEY_IGNORE_CASE) {
                char *key_lower = duplicate_key_lower(key);
                uint32_t hash = 0;

                hash = table->hash_function(key_lower);

                free(key_lower);

                return hash;
        }

        return table->hash_function(key);
}

struct dict_table *create_map(size_t size)
{
        return create_map_internal(size, DICT_TABLE_IGNORE_THRESHOLDS_EMPTY);
}

struct dict_table *create_map_with_flags(size_t size, uint8_t flags)
{
        return create_map_internal(size,
                                   flags | DICT_TABLE_IGNORE_THRESHOLDS_EMPTY);
}

uint32_t key_hash(const char *key)
{
        // TODO: Maybe grab a seed from env variables for startup
        return hash_murmur3_32(key, strlen(key), 1);
}

void map_set_flags(struct dict_table *table, uint8_t flags)
{
        table->flags = flags;
}
