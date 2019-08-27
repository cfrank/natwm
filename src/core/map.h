// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>

#ifdef USE_POSIX
#include <pthread.h>
#endif

/**
 * The following is a implementation of a simple hash table.
 *
 * It exposes the following:
 *
 * --Constructor--
 *
 * map_init(void)               Creates and returns a new hash map
 *
 * --Destructor--
 *
 * map_destroy(m)               Destroys the map _m_ and frees all of its
 *                              entries
 *
 * map_destroy_function(m, f)   Destroys the map _m_ and frees all of its
 *                              entries using the function _f_
 *
 * --Primary methods--
 *
 * map_insert(m, k, v)          Insert a value _v_ into bucket with key _k_
 *                              inside map _m_
 *
 * map_add(m, k, v)             Add a value _v_ into bucket with key _k_ inside
 *                              map _m_ aborting if there exists a bucket with
 *                              key _k_
 *
 * map_delete(m, k)             Delete a value from bucket with key _k_ inside
 *                              map _m_
 *
 * map_foreach(m, f)            Iterate over the values inside map _m_ calling
 *                              function _f_ with the values key and value
 *
 * --Setters--
 *
 * map_set_hash_function(m, f)  Set hash function _f_ for map _m_ to use when
 *                              hashing keys
 *
 * map_set_free_function(m, f)  Set free function _f_ for map _m_ to use when
 *                              freeing entries
 *
 * map_set_setting_flag(m, f)   Set setting flag _f_ for map _m_
 *
 *
 * The implementation details of the hash table are as follows:
 *
 * The hash table uses the 32 bit version of the murmur hash v3
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
 * table size
 *
 * Table size is bound to a set defined as {log(MAP_MIN_SIZE)^2 ... n^2}
 *
 * Keys are pointers to char
 *
 * Values are pointers to void
 */

// Takes a key and returns a hash
typedef uint32_t (*map_hash_function_t)(const char *key);

#define MAP_MIN_LENGTH 4
#define MAP_LOAD_FACTOR_HIGH 0.75
#define MAP_LOAD_FACTOR_LOW 0.2

enum map_settings {
        MAP_FLAG_KEY_IGNORE_CASE = 1 << 0, // Ignore casing for keys
        MAP_FLAG_USE_FREE = 1 << 1, // Use free instead of supplied free func
        MAP_FLAG_IGNORE_THRESHOLDS = 1 << 2, // Ignore load factors
        MAP_FLAG_IGNORE_THRESHOLDS_EMPTY = 1 << 3, // Ignore low_load_factor
        MAP_FLAG_NO_LOCKING = 1 << 4, // Don't try to be thread safe
};

enum map_events {
        EVENT_FLAG_NORMAL = 1 << 0, // Nothing special
        EVENT_FLAG_REHASHING_MAP = 1 << 1, // Map is currently rehashing
        EVENT_FLAG_ITERATING = 1 << 2, // Map is currently iterating
};

// Represents a entry in the hash table
struct dict_entry {
        // Tradeoff: Getting DIB from hash instead of storing DIB in entry
        // Pro: Smaller entry memory footprint
        // Con: Increased computation when calculating DIB
        uint32_t hash;
        char *key;
        void *data;
};

// Represents a hash table
struct dict_map {
        uint32_t length; // Length of the table (power of 2)
        uint32_t bucket_count;
        struct dict_entry **entries;
#ifdef USE_POSIX
        pthread_mutex_t mutex;
#endif
        map_hash_function_t hash_function;
        enum map_settings setting_flags;
        enum map_events event_flags;
};

// Frees data stored in the map entry
typedef void (*map_entry_free_function_t)(void *data);

// Callback function for iterator
typedef void (*map_foreach_callback_function_t)(const struct dict_entry *entry);

struct dict_map *map_init(void);
void map_destroy(struct dict_map *map);
void map_destroy_func(struct dict_map *map,
                      const map_entry_free_function_t free_function);

int map_insert(struct dict_map *map, char *key, void *data);
int map_add(struct dict_map *map, char *key, void *data);
int map_delete(struct dict_map *map, const char *key);

void map_foreach(const struct dict_map *map,
                 const map_foreach_callback_function_t callback);

void map_set_hash_function(struct dict_map *map,
                           const map_hash_function_t function);
void map_set_setting_flag(struct dict_map *map, enum map_settings flag);
