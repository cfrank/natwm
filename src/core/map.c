// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/hash.h>
#include "map.h"

// Default map hashing function
// Uses Murmur v3 32bit
static ATTR_CONST uint32_t default_key_hash(const char *key)
{
        // TODO: Should thought be put into a seed for the hash
        return hash_murmur3_32(key, strlen(key), 0);
}

// Given a power of 2 - find the next power of 2
static ATTR_INLINE ATTR_CONST uint32_t next_power(uint32_t num)
{
        return num * 2;
}

// Given a power of 2 - find the previous power of 2
static ATTR_INLINE ATTR_CONST uint32_t previous_power(uint32_t num)
{
        return (num * 0x200001) >> 22;
}

// Given a hash, key, and value construct a map entry
static enum map_error entry_init(uint32_t hash, const char *key, void *value,
                                 struct map_entry **dest)
{
        *dest = malloc(sizeof(struct map_entry));

        if (*dest == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        (*dest)->hash = hash;
        (*dest)->key = key;
        (*dest)->value = value;

        return NO_ERROR;
}

// Given a map_entry determine if it holds valid data and is present
static int entry_is_present(const struct map_entry *entry)
{
        if (entry != NULL && entry->key != NULL && entry->value != NULL) {
                return 0;
        }

        return -1;
}

// Given a map_entry and it's current index find the distance from the initial
// bucket
static uint32_t get_dib(const struct map *map, const struct map_entry *entry,
                        uint32_t current_index)
{
        uint32_t initial_index = entry->hash % map->length;

        if (current_index < initial_index) {
                return (map->length - current_index) + initial_index;
        }

        return current_index - initial_index;
}

// Attempt to lock the map
static int map_lock(struct map *map)
{
        assert(map);

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

// Attempt to unlock the map
static int map_unlock(struct map *map)
{
        assert(map);

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

// Use the robin hood hashing method to probe the map for a sutable location to
// place the provided entry
static enum map_error map_probe(struct map *map, struct map_entry *entry,
                                uint32_t initial_index)
{
        // As we probe through the map these will continually get updated
        uint32_t probe_position = initial_index;
        struct map_entry *insert_entry = entry;

        for (;;) {
                if (probe_position == map->length) {
                        probe_position = 0;
                }

                assert(probe_position < map->length);

                struct map_entry *current_entry = map->entries[probe_position];

                if (entry_is_present(current_entry) != 0) {
                        // Insert here
                        map->entries[probe_position] = insert_entry;

                        return NO_ERROR;
                }

                uint32_t insert_dib
                        = get_dib(map, insert_entry, probe_position);

                uint32_t current_dib
                        = get_dib(map, current_entry, probe_position);

                // If the current entry has a lower dib then the entry we are
                // trying to insert then we swap the entries
                if (current_dib < insert_dib) {
                        map->entries[probe_position] = insert_entry;

                        insert_entry = current_entry;
                        probe_position += 1;

                        continue;
                }

                // Keep probing
                probe_position += 1;
        }

        // Should never happen
        return GENERIC_ERROR;
}

// Inserts a pre-hashed entry into the map
//
// Note: This does not check for load factor compliance.
static enum map_error map_insert_entry(struct map *map, struct map_entry *entry)
{
        // Get the new initial index
        uint32_t initial_index = entry->hash % map->length;

        assert(initial_index < map->length);

        if (entry_is_present(map->entries[initial_index]) != 0) {
                // We just insert here
                map->entries[initial_index] = entry;
                map->bucket_count += 1;

                return NO_ERROR;
        }

        // Handle collision
        enum map_error error = map_probe(map, entry, initial_index);

        if (error != NO_ERROR) {
                return error;
        }

        map->bucket_count += 1;

        return NO_ERROR;
}

// Resizes the map either to a smaller size or a larger size
//
// resize_direction == 1 -> Increase size
// resize_direction == -1 -> Decrease size
static enum map_error map_resize(struct map *map, int resize_direction)
{
        uint32_t new_length = map->length;

        if (resize_direction == 1) {
                new_length = next_power(map->length);
        } else {
                new_length = previous_power(map->length);
        }

        struct map_entry **new_entries
                = calloc(new_length, sizeof(struct map_entry *));

        if (new_entries == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        map_lock(map);
        map->event_flags |= EVENT_FLAG_RESIZING_MAP;

        // Cache old entries
        struct map_entry **old_entries
                = malloc(sizeof(struct map_entry *) * map->length);
        size_t old_length = map->length;

        if (old_entries == NULL) {
                // Need to free newly initialized entries
                free(new_entries);

                return MEMORY_ALLOCATION_ERROR;
        }

        memcpy(old_entries,
               map->entries,
               sizeof(struct map_entry *) * map->length);

        // Need to free the old entries
        free(map->entries);

        map->length = new_length;
        map->bucket_count = 0;
        map->entries = new_entries;

        for (size_t i = 0; i < old_length; ++i) {
                struct map_entry *entry = old_entries[i];

                if (entry_is_present(entry) == -1) {
                        // Nothing here

                        continue;
                }

                enum map_error error = map_insert_entry(map, entry);

                if (error != NO_ERROR) {
                        return error;
                }
        }

        free(old_entries);

        map_unlock(map);
        map->event_flags &= (unsigned int)~EVENT_FLAG_RESIZING_MAP;

        return NO_ERROR;
}

// Handle load factor for inserting/removing values
static int get_resize_direction(struct map *map, double new_size)
{
        if (map->setting_flags & MAP_FLAG_IGNORE_THRESHOLDS) {
                return 0;
        }

        double load_factor = new_size / map->length;

        if (load_factor >= MAP_LOAD_FACTOR_HIGH) {
                return MAP_RESIZE_UP;
        }

        if (load_factor <= MAP_LOAD_FACTOR_LOW) {
                if (map->setting_flags & MAP_FLAG_IGNORE_THRESHOLDS_EMPTY) {
                        return 0;
                }

                return MAP_RESIZE_DOWN;
        }

        // Should never happen
        return 0;
}

// Initialize a map
struct map *map_init(void)
{
        struct map *map = malloc(sizeof(struct map));

        if (map == NULL) {
                return NULL;
        }

        map->length = MAP_MIN_LENGTH;
        map->bucket_count = 0;
        map->entries = calloc(map->length, sizeof(struct map_entry));

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

// Destroys a map and it's entries
//
// Note: If the value stored in the entires has nested items which need to be
// free'd then there will be memory leaks if this function is used since we are
// only dealing with the top level. map_destroy_func allows for customizing the
// free'ing of entries
void map_destroy(struct map *map)
{
        // Delete entries
        for (size_t i = 0; i < map->length; ++i) {
                struct map_entry *entry = map->entries[i];

                if (entry_is_present(entry) != -1) {
                        if (map->setting_flags & MAP_FLAG_USE_FREE) {
                                free(entry->value);
                        }

                        free(entry);
                }
        }

        free(map->entries);
        free(map);
}

// Destroys a map using a custom free function
void map_destroy_func(struct map *map,
                      const map_entry_free_function_t free_function)
{
        for (size_t i = 0; i < map->length; ++i) {
                struct map_entry *entry = map->entries[i];

                if (entry_is_present(entry) != -1) {
                        free_function(entry);
                }
        }

        free(map->entries);
        free(map);
}

// Insert an entry into a map
enum map_error map_insert(struct map *map, const char *key, void *value)
{
        int resize_dir = get_resize_direction(map, map->bucket_count + 1.0);

        if (resize_dir != 0) {
                map_resize(map, resize_dir);
        }

        struct map_entry *entry = NULL;
        uint32_t hash = map->hash_function(key);
        enum map_error error = entry_init(hash, key, value, &entry);

        if (error != NO_ERROR) {
                return error;
        }

        return map_insert_entry(map, entry);
}

// Iterate through the map calling the callback for each entry
void map_foreach(const struct map *map,
                 const map_foreach_callback_function_t callback)
{
        for (size_t i = 0; i < map->length; ++i) {
                struct map_entry *entry = map->entries[i];

                if (entry_is_present(entry) != -1) {
                        callback(entry);
                }
        }
}
