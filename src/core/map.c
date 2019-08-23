// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <common/constants.h>
#include <common/hash.h>
#include "map.h"

static ATTR_CONST uint32_t default_key_hash(const char *key)
{
        // TODO: Should any thought be put into a seed
        return hash_murmur3_32(key, strlen(key), 0);
}

static struct dict_entry *entry_init(uint32_t hash, char *key, void *data)
{
        struct dict_entry *entry = malloc(sizeof(struct dict_entry));

        if (entry == NULL) {
                return NULL;
        }

        entry->hash = hash;
        entry->key = key;
        entry->data = data;

        return entry;
}

static int entry_is_present(const struct dict_entry *entry)
{
        return entry->key != NULL && entry->data != NULL;
}

static ATTR_INLINE size_t get_dib(const struct dict_map *map,
                                  const struct dict_entry *entry,
                                  uint32_t current_index)
{
        uint32_t initial_index = entry->hash % map->length;

        return current_index - initial_index;
}

static int map_probe(struct dict_map *map, const struct dict_entry *entry,
                     uint32_t initial_index)
{
        uint32_t probe_position = initial_index + 1;
        uint32_t dib = 1;

        for (;;) {
                if (probe_positon >= map->length) {
                        probe_position = 0;
                }

                struct dict_entry *entry_compare = map->entries[probe_position];
        }
}

struct dict_map *map_init(void)
{
        struct dict_map *map = malloc(sizeof(struct dict_map));

        if (map == NULL) {
                return NULL;
        }

        map->length = MAP_MIN_LENGTH;
        map->bucket_count = 0;
        map->buckets = calloc(map->length, sizeof(struct dict_entry));

        if (map->buckets == NULL) {
                return NULL;
        }

#ifdef USE_POSIX
        if (pthread_mutex_init(map->mutex) != 0) {
                return NULL;
        }
#endif
        map->hash_function = default_key_hash;
        map->setting_flags = MAP_FLAG_IGNORE_THRESHOLDS_EMPTY;
        map->event_flags = EVENT_FLAG_NORMAL;

        return map;
}

/**
 * Deletes a map by free'ing all entries and free'ing the table
 *
 * NOTE: If the data stored in the entries has nested items which need
 * to be free'd then there will be memory leaks if this function is used
 * since we are only dealing with the first level. map_destroy_func
 * allows for customized free'ing of entries.
 */
void map_destroy(struct dict_map *map)
{
        // Delete entries
        for (size_t i = 0; i < map->length; ++i) {
                struct dict_entry *entry = map->entries[i];

                if (entry_is_present(entry)) {
                        free(map->entries[i]->data);
                        free(map->entries[i]);
                }
        }

        free(map);
}

/**
 * Deletes a map by calling a provided free_function on all the data held
 * by every entry.
 *
 * NOTE: Use this if you have data which has nested allocs
 */
void map_destroy_func(struct dict_map *map,
                      const map_entry_free_function_t free_function)
{
        for (size_t i = 0; i < map->length; ++i) {
                struct dict_entry *entry = map->entries[i];

                if (entry_is_present(entry)) {
                        free_function(map->entries[i]->data);
                        free(map->entries[i]);
                }
        }

        free(map);
}

int map_insert(struct dict_map *map, char *key, void *data)
{
        assert(map);

        const double load_factor = (map->bucket_count + 1) / map->length;

        // Determine if a resize is required
        if (load_factor > MAP_LOAD_FACTOR_HIGH) {
                if (load_factor > 1) {
                        // Can't insert into a full table
                        return -1;
                }

                // Only resize if we aren't ignoring thresholds
                if (!map->flags & MAP_FLAG_IGNORE_THRESHOLDS) {
                        // TODO: Resize
                }
        }

        uint32_t hash = map->hash_function(key);
        int32_t initial_index = hash % map->length;

        assert(original_index < map->length);

        if (entry_is_present(map->entries[initial_index])) {
                // We have encountered a collision start probe
                return -1;
        }

        struct dict_entry *entry = entry_init(hash, key, data);

        if (entry == NULL) {
                return -1;
        }

        map->entries[initial_index] = entry;

        return 0;
}

void map_foreach(const struct dict_map *map,
                 const map_foreach_callback_t callback)
{
        for (size_t i = 0; i < map->length; ++i) {
                struct dict_entry entry = map->buckets[i];

                if (entry_is_present(entry)) {
                        callback(entry);
                }
        }
}
