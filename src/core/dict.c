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

static ATTR_CONST ATTR_INLINE uint32_t next_power(uint32_t num)
{
        uint32_t i = MAP_MIN_SIZE;

        while (i < num) {
                i <<= 1;
        }

        return i;
}

static ATTR_CONST ATTR_INLINE uint32_t previous_power(uint32_t num)
{
        num = num | (num >> 1);
        num = num | (num >> 2);
        num = num | (num >> 4);
        num = num | (num >> 8);
        num = num | (num >> 16);

        return num - (num >> 1);
}

/**
 * Check to see if the map needs to resized - either up or down
 *
 * Returns either the next or previous power if it needs to be resized.
 * Otherwise mpa->size
 */
static uint32_t map_needs_resizing(const struct dict_map *map)
{
        if (map->size == MAP_MIN_SIZE) {
                return map->size;
        }

        const float current_ratio = map->bucket_count / map->size;

        // First check if we have too many entries
        if (current_ratio > map->high_load_factor) {
                return next_power(map->size);
        }

        // Next check if we have too few entries
        if (current_ratio < map->low_load_factor) {
                return previous_power(map->size - 1);
        }

        return map->size;
}

static ATTR_INLINE int lock_map(struct dict_map *map)
{
        if (map->flags & MAP_FLAG_NO_LOCKING) {
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
        if (map->flags & MAP_FLAG_NO_LOCKING) {
                return -1;
        }

#ifdef USE_POSIX
        if (pthread_mutex_unlock(&map->mutex) != 0) {
                return -1;
        }

        return 0;
#endif
        return -1;
}

/**
 * Take a string and return the hash value for it.
 *
 * This returns the raw hash and must be mapped to the table size
 */
static ATTR_INLINE uint32_t hash_value(const struct dict_map *map,
                                       const char *key)
{
        if (map->flags & MAP_FLAG_KEY_IGNORE_CASE) {
                char *key_lower = duplicate_key_lower(key);
                uint32_t hash = 0;

                hash = map->hash_function(key_lower);

                free(key_lower);

                return hash;
        }

        return map->hash_function(key);
}

static struct dict_entry *create_entry(const char *key, void *data,
                                       size_t data_size)
{
        struct dict_entry *entry = malloc(sizeof(struct dict_entry));

        if (entry == NULL) {
                return NULL;
        }

        entry->key = alloc_string(key);

        if (entry->key == NULL) {
                return NULL;
        }

        entry->data = data;
        entry->data_size = data_size;
        entry->next = NULL;

        return entry;
}

static struct dict_map *create_map_internal(size_t size, uint8_t flags)
{
        struct dict_map *map = malloc(sizeof(struct dict_map));

        if (map == NULL) {
                return NULL;
        }

        map->size = next_power((uint32_t)size);
        map->bucket_count = 0;
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

struct dict_map *create_map(size_t size)
{
        return create_map_internal(size, MAP_FLAG_IGNORE_THRESHOLDS_EMPTY);
}

struct dict_map *create_map_with_flags(size_t size, uint8_t flags)
{
        return create_map_internal(size,
                                   flags | MAP_FLAG_IGNORE_THRESHOLDS_EMPTY);
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
        if (map->bucket_count > 0) {
                return -1;
        }

        map->hash_function = func;

        return 0;
}

void map_set_free_function(struct dict_map *map, dict_free_function_t func)
{
        map->free_function = func;
}
