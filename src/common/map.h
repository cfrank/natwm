// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <pthread.h>
#include <stdint.h>

#include <common/error.h>

/**
 * The following is a implementation of a simple hash map.
 *
 * The implementation details are as follows:
 *
 * The hash mamapp uses the 32 bit version of the murmur hash v3 by default
 *
 * Collisions are resolved using open addressing with the robin hood hashing
 * algorithm - probing linearly
 *
 * Deletion is performed using Emmanuel Goossaert's backward shift deletion
 * algorithm
 *
 * The table implements bi-directional resizing using high+low load-factors
 *
 * After resize a re-hash is performed to amortize the keys across the new
 * map size
 *
 * Keys are pointers to char
 *
 * Values are pointers to void
 */

// Function which takes a key and returns a hash
typedef uint32_t (*map_hash_function_t)(const void *key, size_t size);

// Function for finding the size of a supplied key
typedef size_t (*map_key_size_function_t)(const void *key);

// Function which takes data and frees the memory allocated for it
typedef void (*map_entry_free_function_t)(void *data);

#define MAP_MIN_LENGTH 4
#define MAP_LOAD_FACTOR_HIGH 0.75
#define MAP_LOAD_FACTOR_LOW 0.2
#define MAP_RESIZE_UP 1
#define MAP_RESIZE_DOWN -1

enum map_settings {
        MAP_FLAG_KEY_IGNORE_CASE = 1 << 0, // Ignore casing for keys
        MAP_FLAG_USE_FREE = 1 << 1, // Use free instead of supplied free func
        MAP_FLAG_FREE_ENTRY_KEY = 1 << 2, // Free the entry key
        MAP_FLAG_USE_FREE_FUNC = 1 << 3, // Use a custom free function
        MAP_FLAG_IGNORE_THRESHOLDS = 1 << 4, // Ignore load factors
        MAP_FLAG_IGNORE_THRESHOLDS_EMPTY = 1 << 5, // Ignore low_load_factor
        MAP_FLAG_NO_LOCKING = 1 << 6, // Don't try to be thread safe
};

enum map_events {
        EVENT_FLAG_NORMAL = 1 << 0, // Nothing special
        EVENT_FLAG_RESIZING_MAP = 1 << 1, // Map is currently resizing
        EVENT_FLAG_ITERATING = 1 << 2, // Map is currently iterating
};

// Represents an entry in the hash map
struct map_entry {
        // Tradeoff: Getting DIB from hash instead of storing DIB in entry
        // Pro: Smaller entry memory footprint
        // Con: Increased computation when calculating DIB
        uint32_t hash;
        const void *key;
        void *value;
};

// Represents a hash map
struct map {
        uint32_t length; // Length of the map (power of 2)
        uint32_t bucket_count;
        struct map_entry **entries;
        pthread_mutex_t mutex;
        map_hash_function_t hash_function;
        map_key_size_function_t key_size_function;
        map_entry_free_function_t free_function;
        enum map_settings setting_flags;
        enum map_events event_flags;
};

enum natwm_error entry_init(uint32_t hash, const void *key, void *value,
                            struct map_entry **dest);
void map_entry_destroy(const struct map *map, struct map_entry *entry);

struct map *map_init(void);
void map_destroy(struct map *map);
enum natwm_error map_insert(struct map *map, const void *key, void *value);
struct map_entry *map_get(const struct map *map, const void *key);
enum natwm_error map_delete(struct map *map, const void *key);

int map_set_hash_function(struct map *map, map_hash_function_t function);
int map_set_key_size_function(struct map *map,
                              map_key_size_function_t function);
void map_set_entry_free_function(struct map *map,
                                 map_entry_free_function_t function);
void map_set_setting_flag(struct map *map, enum map_settings flag);
void map_remove_setting_flag(struct map *map, enum map_settings flag);

uint32_t map_get_uint32(const struct map *map, const void *key,
                        enum natwm_error *error);
