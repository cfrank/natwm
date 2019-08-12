// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <common/hash.h>
#include "dict.h"

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
