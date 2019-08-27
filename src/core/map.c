// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/hash.h>
#include "map.h"

static ATTR_CONST uint32_t default_key_hash(const char *key)
{
        // TODO: Should any thought be put into a seed
        return hash_murmur3_32(key, strlen(key), 0);
}

/**
 * Given a power of 2, find the next power of 2
 */
static ATTR_INLINE ATTR_CONST uint32_t next_power(uint32_t num)
{
        return num * 2;
}

/**
 * Given a power of 2, find the previous power of 2
 */
static ATTR_INLINE ATTR_CONST uint32_t previous_power(uint32_t num)
{
        return (num * 0x200001) >> 22;
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

static ATTR_INLINE int entry_is_present(const struct dict_entry *entry)
{
        return entry != NULL && entry->key != NULL && entry->data != NULL;
}

static ATTR_INLINE uint32_t get_dib(const struct dict_map *map,
                                    const struct dict_entry *entry,
                                    uint32_t current_index)
{
        uint32_t initial_index = entry->hash % map->length;

        if (current_index < initial_index) {
                return (map->length - current_index) + initial_index;
        }

        return current_index - initial_index;
}

static int map_lock(struct dict_map *map)
{
        if (!map) {
                return -1;
        }

        if (map->setting_flags & MAP_FLAG_NO_LOCKING) {
                return 0;
        }

#ifdef USE_POSIX
        if (pthread_mutex_lock(&map->mutex) != 0) {
                return -1;
        }
#endif
        return 0;
}

static int map_unlock(struct dict_map *map)
{
        if (!map) {
                return -1;
        }

        if (map->setting_flags & MAP_FLAG_NO_LOCKING) {
                return 0;
        }

#ifdef USE_POSIX
        if (pthread_mutex_unlock(&map->mutex) != 0) {
                return -1;
        }
#endif
        return 0;
}

static int map_probe(struct dict_map *map, struct dict_entry *entry,
                     uint32_t initial_index)
{
        uint32_t probe_position = initial_index;
        struct dict_entry *insert_entry = entry;

        for (size_t i = 0; i < map->length; ++i) {
                if (probe_position <= map->length) {
                        probe_position = 0;
                }

                struct dict_entry *current_entry = map->entries[probe_position];

                if (!entry_is_present(current_entry)) {
                        // Insert here
                        map->entries[probe_position] = insert_entry;

                        return 0;
                }

                uint32_t insert_dib
                        = get_dib(map, insert_entry, probe_position);

                assert(insert_dib < map->length);

                uint32_t current_dib
                        = get_dib(map, current_entry, probe_position);

                if (current_dib < insert_dib) {
                        // Swap
                        map->entries[probe_position] = insert_entry;

                        insert_entry = current_entry;
                        probe_position = probe_position + 1;

                        continue;
                }

                // Keep probing
                probe_position = probe_position + 1;
        }

        // Should never happen
        return -1;
}

/**
 * Inserts a pre-hashed dict_entry into a dict_map.
 *
 * NOTE: This does not check for load factor compliance. It is an internal
 * function which is used by the exported map_insert and map_resize. Both
 * of these function deal with load_factor in their own ways, but when
 * calling this function it should be known that the load factor will be
 * kept valid.
 */
static int map_insert_entry(struct dict_map *map, struct dict_entry *entry)
{
        uint32_t initial_index = entry->hash % map->length;

        assert(initial_index < map->length);
        assert(entry != NULL);

        if (entry_is_present(map->entries[initial_index])) {
                // Handle collision
                if (map_probe(map, entry, initial_index) != 0) {
                        return -1;
                }

                return 0;
        }

        map->entries[initial_index] = entry;

        return 0;
}

static int map_resize(struct dict_map *map, int resize_direction)
{
        uint32_t new_length = 0;

        if (resize_direction == 1) {
                new_length = next_power(map->length);
        } else {
                new_length = previous_power(map->length);
        }

        assert(map->length < new_length);

        struct dict_entry **new_entries
                = calloc(new_length, sizeof(struct dict_entry *));

        if (new_entries == NULL) {
                return -1;
        }

        map_lock(map);

        // Resize map to new_size
        struct dict_map old_map = *map;

        map->bucket_count = new_length;
        map->entries = new_entries;

        for (size_t i = 0; i < old_map.length; ++i) {
                struct dict_entry *entry = old_map.entries[i];

                if (!entry_is_present(entry)) {
                        // Nothing here - continue
                        continue;
                }

                if (map_insert_entry(map, entry) != 0) {
                        return -1;
                }
        }

        map_unlock(map);

        return 0;
}

struct dict_map *map_init(void)
{
        struct dict_map *map = malloc(sizeof(struct dict_map));

        if (map == NULL) {
                return NULL;
        }

        map->length = MAP_MIN_LENGTH;
        map->bucket_count = 0;
        map->entries = calloc(map->length, sizeof(struct dict_entry));

        if (map->entries == NULL) {
                return NULL;
        }

#ifdef USE_POSIX
        if (pthread_mutex_init(&map->mutex, NULL) != 0) {
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
                        if (map->setting_flags & MAP_FLAG_USE_FREE) {
                                free(map->entries[i]->data);
                        }
                        free(map->entries[i]);
                }
        }

        free(map->entries);
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

        double load_factor = (map->bucket_count + 1) / map->length;

        // Determine if a resize is required
        if (load_factor > MAP_LOAD_FACTOR_HIGH) {
                // Only resize if we aren't ignoring thresholds
                if (!(map->setting_flags & MAP_FLAG_IGNORE_THRESHOLDS)) {
                        // TODO: Resize
                        map_resize(map, 1);
                }
        }

        uint32_t hash = map->hash_function(key);
        struct dict_entry *entry = entry_init(hash, key, data);

        if (entry == NULL) {
                return -1;
        }

        return map_insert_entry(map, entry);
}

void map_foreach(const struct dict_map *map,
                 const map_foreach_callback_function_t callback)
{
        for (size_t i = 0; i < map->length; ++i) {
                struct dict_entry *entry = map->entries[i];

                if (entry_is_present(entry)) {
                        callback(entry);
                }
        }
}
