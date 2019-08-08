// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"

/**
 * Use perl's hashing function by default
 *
 * Customizing is provided by the hash_function property on dict_table
 */
static uint32_t hash_function(const void *key, size_t key_size)
{
        register size_t i = key_size;
        register const uint8_t *s = (const uint8_t *)key;
        register uint32_t ret = 0;

        while (i--) {
                ret += *s++;
                ret += (ret << 10);
                ret ^= (ret >> 6);
        }

        ret += (ret << 3);
        ret ^= (ret >> 11);
        ret += (ret << 15);

        return ret;
}

static ATTR_INLINE void *duplicate_key(const void *key, size_t key_size)
{
        void *new_key = malloc(key_size);

        if (new_key == NULL) {
                return NULL;
        }

        memcpy(new_key, key, key_size);

        return new_key;
}

static ATTR_INLINE void *duplicate_key_lower(const void *key, size_t key_size)
{
        char *new_key = (char *)duplicate_key(key, key_size);

        if (new_key == NULL) {
                return NULL;
        }

        for (size_t i = 0; i < key_size; ++i) {
                new_key[i] = (char)tolower(new_key[i]);
        }

        return new_key;
}

static ATTR_INLINE uint32_t get_power_two(uint32_t num)
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
        pthread_mutex_init(&table->mutex, NULL);
#endif
        table->flags = flags;
        table->hash_function = hash_function;
        table->itr_bucket_index = 0;
        table->high_load_factor = 0.75;
        table->low_load_factor = 0.20;
        table->free_function = NULL;
        table->event_flags = 0;

        return table;
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

void map_set_flags(struct dict_table *table, uint8_t flags)
{
        table->flags = flags;
}
