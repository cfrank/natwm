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
typedef unsigned int (*dict_hash_function_t)(const void *key, size_t key_size);

// Frees data stored in the dict
typedef void (*dict_free_function_t)(void *data);

// Exported flags for the table
#define DICT_KEY_IGNORE_CASE (1 << 0) // Ignore casing for keys
#define DICT_TABLE_USE_FREE (1 << 1) // Call free() on value when destroying
#define DICT_TABLE_IGNORE_THRESHOLDS (1 << 2) // Ignore load factors
#define DICT_TABLE_IGNORE_THRESHOLDS_EMPTY (1 << 3) // Ignore low_load_factor
#define DICT_TABLE_NO_LOCKING (1 << 4) // Don't be thread safe

struct dict_entry {
        void *key;
        size_t key_size;
        void *data;
        size_t data_size;
        struct dict_entry *next; // Used for iterating
};

struct dict_table {
        size_t bucket_count;
        size_t entries_count;
        struct dict_entry **entries;
#ifdef USE_POSIX
        pthread_mutex_t mutex;
#endif
        uint8_t flags;
        dict_hash_function_t hash_function;
        size_t itr_bucket_index; // The current index during iteration
        double high_load_factor;
        double low_load_factor;
        dict_free_function_t free_function;
        uint32_t resize_count;
        uint8_t event_flags;
};

struct dict_table *create_map(size_t size);
struct dict_table *create_map_with_flags(size_t size, uint8_t flags);
void map_set_flags(struct dict_table *table, uint8_t flags);
