// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stdint.h>
#ifdef USE_POSIX
#include <pthread.h>
#endif

#include <common/constants.h>

// Takes a key and returns a hash
typedef uint32_t (*dict_hash_function_t)(const char *key);

// Frees data stored in the dict
typedef void (*dict_free_function_t)(void *data);

// Exported flags for the map
#define MAP_FLAG_KEY_IGNORE_CASE (1 << 0) // Ignore casing for keys
#define MAP_FLAG_USE_FREE (1 << 1) // Call free() on value when destroying
#define MAP_FLAG_IGNORE_THRESHOLDS (1 << 2) // Ignore load factors
#define MAP_FLAG_IGNORE_THRESHOLDS_EMPTY (1 << 3) // Ignore low_load_factor
#define MAP_FLAG_NO_LOCKING (1 << 4) // Don't be thread safe

// Exported event flags for the map
#define EVENT_FLAG_REHASHING_MAP (1 << 0) // Map is currently rehashing
#define EVENT_FLAG_ITERATING (1 << 1) // Iteration in progress

// Size constants
#define MAP_MIN_SIZE_MASK 4

struct dict_entry {
        const char *key;
        void *data;
        size_t data_size;
        struct dict_entry *next;
};

struct dict_map {
        uint32_t size_mask; // Power of two max size of the map
        uint32_t bucket_count; // Current number of buckets in the map
        struct dict_entry **entries;
#ifdef USE_POSIX
        pthread_mutex_t mutex;
#endif
        dict_hash_function_t hash_function; // Should never be NULL
        dict_free_function_t free_function; // If NULL free(1) will be used
        size_t itr_bucket_index; // The current index during iteration
        double high_load_factor;
        double low_load_factor;
        size_t resize_count;
        uint8_t flags; // Map settings flags
        uint8_t event_flags; // Map event flags
};

struct dict_map *create_map(size_t size);
struct dict_map *create_map_with_flags(size_t size, uint8_t flags);
uint32_t key_hash(const char *key);
void map_set_flags(struct dict_map *map, uint8_t flags);
void map_clear_flag(struct dict_map *map, uint8_t flag);
int map_set_hash_function(struct dict_map *map, dict_hash_function_t func);
void map_set_free_function(struct dict_map *map, dict_free_function_t func);
