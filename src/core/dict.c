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

static ATTR_INLINE int lock_map(struct dict_map *map)
{
        assert(map);

        if (map->flags & DICT_MAP_NO_LOCKING) {
                return -1;
        }
#ifdef USE_POSIX
        if (pthread_mutex_lock(&map->mutex) != 0) {
                return -1;
        }

        return 0;
#endif
        return -1;
}

static ATTR_INLINE int unlock_map(struct dict_map *map)
{
        assert(map);
}

static struct dict_map *create_map_internal(size_t size, uint8_t flags)
{
        struct dict_map *map = malloc(sizeof(struct dict_map));

        if (map == NULL) {
                return NULL;
        }

        map->bucket_count = get_power_two((uint32_t)size);
        map->entries_count = 0;
        map->entries = NULL;
#ifdef USE_POSIX
        if (pthread_mutex_init(&map->mutex, NULL) != 0) {
                return NULL;
        }
#endif
        map->flags = flags;
        map->hash_function = key_hash;
        map->itr_bucket_index = 0;
        map->high_load_factor = 0.75;
        map->low_load_factor = 0.20;
        map->free_function = NULL;
        map->event_flags = 0;

        return map;
}

static ATTR_INLINE uint32_t hash_value(const struct dict_map *map,
                                       const char *key)
{
        assert(key);

        if (map->flags & DICT_KEY_IGNORE_CASE) {
                char *key_lower = duplicate_key_lower(key);
                uint32_t hash = 0;

                hash = map->hash_function(key_lower);

                free(key_lower);

                return hash;
        }

        return map->hash_function(key);
}

struct dict_map *create_map(size_t size)
{
        return create_map_internal(size, DICT_MAP_IGNORE_THRESHOLDS_EMPTY);
}

struct dict_map *create_map_with_flags(size_t size, uint8_t flags)
{
        return create_map_internal(size,
                                   flags | DICT_MAP_IGNORE_THRESHOLDS_EMPTY);
}

uint32_t key_hash(const char *key)
{
        // TODO: Maybe grab a seed from env variables for startup
        return hash_murmur3_32(key, strlen(key), 1);
}

void map_set_flags(struct dict_map *map, uint8_t flags)
{
        map->flags = flags;
}

void map_clear_flag(struct dict_map *map, uint8_t flag)
{
        map->flags &= (uint8_t)(~(1 << flag));
}

int map_set_hash_function(struct dict_map *map, dict_hash_function_t func)
{
        assert(func);

        if (map->entries_count > 0) {
                return -1;
        }

        map->hash_function = func;

        return 0;
}

void map_set_free_function(struct dict_map *map, dict_free_function_t func)
{
        assert(func);

        map->free_function = func;
}
