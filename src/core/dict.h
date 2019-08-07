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
        float high_load_factor;
        float low_load_factor;
        dict_free_function_t free_function;
        uint32_t resize_count;
        uint8_t event_flags;
};
