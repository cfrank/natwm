// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/error.h>
#include <common/hash.h>
#include "map.h"

// Forward declarations
static enum natwm_error map_insert_entry(struct map *map,
                                         struct map_entry *entry);

// Default map hashing function
// Uses Murmur v3 32bit
static ATTR_CONST uint32_t default_hash_function(const void *key, size_t size)
{
        // TODO: Should thought be put into a seed for the hash
        return hash_murmur3_32(key, size, 0);
}

// Default key size function
// This will return the correct size of a string key
static size_t default_key_size_function(const void *key)
{
        return strlen(key);
}

// Default key compare function
// This will compare two strings
static bool default_key_compare_function(const void *one, const void *two,
                                         size_t key_size)
{
        UNUSED_FUNCTION_PARAM(key_size);

        if (strcmp(one, two) == 0) {
                return true;
        }

        return false;
}

// Given a map_entry determine if it holds valid data and is present
static bool is_entry_present(const struct map_entry *entry)
{
        if (entry != NULL && entry->key != NULL) {
                return true;
        }

        return false;
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

        if (pthread_mutex_lock(&map->mutex) != 0) {
                return -1;
        }

        return 0;
}

// Attempt to unlock the map
static int map_unlock(struct map *map)
{
        assert(map);

        if (map->setting_flags & MAP_FLAG_NO_LOCKING) {
                return 0;
        }

        if (pthread_mutex_unlock(&map->mutex) != 0) {
                return -1;
        }

        return 0;
}

// Use the robin hood hashing method to probe the map for a sutable location to
// place the provided entry
static enum natwm_error map_probe(struct map *map, struct map_entry *entry,
                                  uint32_t initial_index)
{
        // As we probe through the map these will continually get updated
        uint32_t probe_position = initial_index;
        struct map_entry *insert_entry = entry;

        for (size_t i = 0; i < map->length; ++i) {
                if (probe_position >= map->length) {
                        probe_position = 0;
                }

                struct map_entry *current_entry = map->entries[probe_position];

                if (!is_entry_present(current_entry)) {
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
                }

                // Keep probing
                probe_position += 1;
        }

        return CAPACITY_ERROR;
}

static enum natwm_error map_search(const struct map *map, const void *key,
                                   uint32_t *index)
{
        if (key == NULL || index == NULL) {
                return GENERIC_ERROR;
        }

        // Initialize index with initial bucket index
        size_t key_size = map->key_size_function(key);
        uint32_t current_index
                = map->hash_function(key, key_size) % map->length;

        for (size_t i = 0; i <= map->length; ++i) {
                if ((current_index) >= map->length) {
                        current_index = 0;
                }

                struct map_entry *entry = map->entries[current_index];

                if (!is_entry_present(entry)
                    || !map->key_compare_function(key, entry->key, key_size)) {
                        current_index += 1;

                        continue;
                }

                *index = current_index;

                return NO_ERROR;
        }

        return NOT_FOUND_ERROR;
}

// Handle load factor for inserting/removing values
static int get_resize_direction(const struct map *map, double new_size)
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

// Resizes the map either to a smaller size or a larger size
//
// resize_direction == 1 -> Increase size
// resize_direction == -1 -> Decrease size
static enum natwm_error map_resize(struct map *map, int resize_direction)
{
        uint32_t new_length = map->length;

        if (resize_direction == 1) {
                new_length = map->length * 2;
        } else if (resize_direction == -1) {
                new_length = map->length / 2;
        } else {
                return GENERIC_ERROR;
        }

        struct map_entry **new_entries
                = calloc(new_length, sizeof(struct map_entry *));

        if (new_entries == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        map_lock(map);
        map->event_flags |= EVENT_FLAG_RESIZING_MAP;

        // Cache old entries
        size_t old_entries_size = sizeof(struct map_entry *) * map->length;
        struct map_entry **old_entries = malloc(old_entries_size);
        size_t old_length = map->length;

        if (old_entries == NULL) {
                // Need to free newly initialized entries array
                free(new_entries);

                return MEMORY_ALLOCATION_ERROR;
        }

        memcpy(old_entries, map->entries, old_entries_size);

        // Need to free the old entries array
        free(map->entries);

        // Set new attributes
        map->length = new_length;
        map->bucket_count = 0;
        map->entries = new_entries;

        for (size_t i = 0; i < old_length; ++i) {
                struct map_entry *entry = old_entries[i];

                if (!is_entry_present(entry)) {
                        // Nothing here

                        continue;
                }

                enum natwm_error error = map_insert_entry(map, entry);

                if (error != NO_ERROR) {
                        return error;
                }
        }

        // Get rid of the cache array
        // Actual entries are stored in the new map->entries array
        free(old_entries);

        map->event_flags &= (unsigned int)~EVENT_FLAG_RESIZING_MAP;
        map_unlock(map);

        return NO_ERROR;
}

// Inserts a pre-hashed entry into the map
static enum natwm_error map_insert_entry(struct map *map,
                                         struct map_entry *entry)
{
        // Get the new initial idex
        uint32_t initial_index = entry->hash % map->length;
        struct map_entry *present_entry = map->entries[initial_index];
        bool is_hash_collision = is_entry_present(present_entry);
        size_t key_size = map->key_size_function(entry->key);

        // If there is a collision with the same key overwrite it
        if (is_hash_collision
            && map->key_compare_function(
                    entry->key, present_entry->key, key_size)) {
                map_entry_destroy(map, present_entry);

                map->entries[initial_index] = entry;

                return NO_ERROR;
        }

        // Now we need to increase map->bucket_count
        int resize_direction
                = get_resize_direction(map, map->bucket_count + 1.0);

        if (resize_direction != 0) {
                enum natwm_error resize_error
                        = map_resize(map, resize_direction);

                if (resize_error != NO_ERROR) {
                        return resize_error;
                }

                // Insert with the new size
                return map_insert_entry(map, entry);
        }

        if (!is_hash_collision) {
                // No collision - just insert here
                map->entries[initial_index] = entry;
                map->bucket_count += 1;

                return NO_ERROR;
        }

        if ((map->bucket_count + 1) > map->length) {
                map_entry_destroy(map, entry);

                return CAPACITY_ERROR;
        }

        // Handle collision
        enum natwm_error error = map_probe(map, entry, initial_index);

        if (error != NO_ERROR) {
                return error;
        }

        map->bucket_count += 1;

        return NO_ERROR;
}

// Given a hash, key, and value construct a map entry
enum natwm_error entry_init(uint32_t hash, const void *key, void *value,
                            struct map_entry **dest)
{
        if (key == NULL || dest == NULL) {
                return GENERIC_ERROR;
        }

        *dest = malloc(sizeof(struct map_entry));

        if (*dest == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        (*dest)->hash = hash;
        (*dest)->key = key;
        (*dest)->value = value;

        return NO_ERROR;
}

void map_entry_destroy(const struct map *map, struct map_entry *entry)
{
        if (is_entry_present(entry)) {
                if (map->setting_flags & MAP_FLAG_FREE_ENTRY_KEY) {
                        free((char *)entry->key);
                }

                if (map->setting_flags & MAP_FLAG_USE_FREE) {
                        free(entry->value);
                }

                if (map->setting_flags & MAP_FLAG_USE_FREE_FUNC) {
                        if (map->free_function == NULL) {
                                return;
                        }

                        map->free_function(entry->value);
                }
        }

        free(entry);
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

        if (pthread_mutex_init(&map->mutex, NULL) != 0) {
                return NULL;
        }

        map->hash_function = default_hash_function;
        map->key_size_function = default_key_size_function;
        map->key_compare_function = default_key_compare_function;
        map->free_function = NULL;
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
        if (map == NULL) {
                return;
        }

        // Delete entries
        for (size_t i = 0; i < map->length; ++i) {
                map_entry_destroy(map, map->entries[i]);
        }

        free(map->entries);
        free(map);
}

// Insert an entry into a map
enum natwm_error map_insert(struct map *map, const void *key, void *value)
{
        if (key == NULL) {
                return GENERIC_ERROR;
        }

        struct map_entry *entry = NULL;
        size_t key_size = map->key_size_function(key);
        uint32_t hash = map->hash_function(key, key_size);
        enum natwm_error error = entry_init(hash, key, value, &entry);

        if (error != NO_ERROR) {
                return error;
        }

        return map_insert_entry(map, entry);
}

struct map_entry *map_get(const struct map *map, const void *key)
{
        uint32_t index = 0;

        if (map_search(map, key, &index) != NO_ERROR) {
                return NULL;
        }

        return map->entries[index];
}

// TODO: Add resize when threshold goes below MAP_LOAD_FACTOR_LOW
enum natwm_error map_delete(struct map *map, const void *key)
{
        uint32_t dest_index = 0;
        enum natwm_error err = map_search(map, key, &dest_index);

        if (err != NO_ERROR) {
                return err;
        }

        uint32_t swap_index = dest_index + 1;

        for (size_t i = 1; i < map->length; ++i) {
                if (swap_index >= map->length) {
                        swap_index = 0;
                }

                struct map_entry *swap_entry = map->entries[swap_index];

                if (!is_entry_present(swap_entry)) {
                        break;
                }

                uint32_t swap_dib = get_dib(map, swap_entry, swap_index);

                if (swap_dib == 0) {
                        break;
                }

                // We need to swap
                struct map_entry *temp = map->entries[dest_index];
                map->entries[dest_index] = swap_entry;
                map->entries[swap_index] = temp;

                // Update values
                dest_index += 1;
                swap_index += 1;
        }

        // Delete the resulting entry
        struct map_entry *delete_entry = map->entries[dest_index];

        // Mark entry as deleted
        delete_entry->key = NULL;
        delete_entry->value = NULL;
        delete_entry = NULL;

        map->bucket_count -= 1;

        return NO_ERROR;
}

// Set a hashing function for use when inserting and re-hashing entries
int map_set_hash_function(struct map *map, map_hash_function_t function)
{
        if (map->bucket_count > 0) {
                // Since we have entries we can't use a different hashing func
                return -1;
        }

        map->hash_function = function;

        return 0;
}

// Set the key sizing function.
int map_set_key_size_function(struct map *map, map_key_size_function_t function)
{
        if (map->bucket_count > 0) {
                // Since we have already added entries to the map we can't
                // change how we determine key size.
                return -1;
        }

        map->key_size_function = function;

        return 0;
}

// Set the key compare function
int map_set_key_compare_function(struct map *map,
                                 map_key_compare_function_t function)
{
        if (map->bucket_count > 0) {
                // Since we have added entries we can't change how we compare
                // keys
                return -1;
        }

        map->key_compare_function = function;

        return 0;
}

// Set a custom free function for use when destroying entries
void map_set_entry_free_function(struct map *map,
                                 map_entry_free_function_t function)
{
        if (map->setting_flags & MAP_FLAG_USE_FREE) {
                map_remove_setting_flag(map, MAP_FLAG_USE_FREE);
        }

        map->free_function = function;

        map_set_setting_flag(map, MAP_FLAG_USE_FREE_FUNC);
}

void map_set_setting_flag(struct map *map, enum map_settings flag)
{
        map->setting_flags |= flag;
}

void map_remove_setting_flag(struct map *map, enum map_settings flag)
{
        map->setting_flags &= (unsigned int)~flag;
}

/**
 * Type specific getters
 */
uint32_t map_get_uint32(const struct map *map, const void *key,
                        enum natwm_error *error)
{
        struct map_entry *entry = map_get(map, key);

        if (entry == NULL) {
                *error = NOT_FOUND_ERROR;
        }

        *error = NO_ERROR;

        return *(uint32_t *)entry->value;
}
