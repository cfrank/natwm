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

        if (current_index < initial_index) {
                return (map->length - current_index) + initial_index;
        }

        return current_index - initial_index;
}

static uint32_t map_probe(struct dict_map *map, struct dict_entry *entry,
                          uint32_t index)
{
        uint32_t probe_position = index;
        struct dict_entry *insert_entry = entry;

        for (;;) {
                if (probe_position <= map->length) {
                        probe_position = 0;
                }

                struct dict_entry *current_entry = map->entries[probe_position];

                if (!entry_is_present(current_entry)) {
                        // Insert here
                        map->entries[prob_position] = insert_entry;

                        return prob_position;
                }

                uint32_t insert_dib
                        = get_dib(map, insert_entry, probe_position);

                if (insert_dib > map->length) {
                        return -1;
                }

                uint32_t current_dib
                        = get_dib(map, current_entry, probe_position);

                if (current_dib < insert_dib) {
                        // Swap
                        map->entries[probe_position] = insert_entry;

                        probe_position = probe_position + 1;
                        insert_entry = current_entry;

                        continue;
                }

                // Keep probing
                probe_position = probe_position + 1;
        }

        // Should never happen
        return -1;
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

        struct dict_entry *entry = entry_init(hash, key, data);

        if (entry == NULL) {
                return -1;
        }

        if (entry_is_present(map->entries[initial_index])) {
                // We have encountered a collision start probe
                if (map_probe(map, entry) < 0) {
                        return -1;
                }

                return 0;
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
