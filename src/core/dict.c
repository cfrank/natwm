// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"

#define ROL32(i32, n) ((i32) << (n) | (i32) >> (32 - (n)))

static ATTR_INLINE uint32_t le32dec(const void *pp)
{
        uint8_t const *p = (uint8_t const *)pp;

        return (uint32_t)((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

/**
 * A simple implementation of the Murmur3-32 hash function
 *
 * Originally written by Austin Appleby https://github.com/aappleby/smhasher
 *
 * C implementation used here taken from the FreeBSD project:
 *
 * Copywrite (c) 2014 Dag-Erling Smørgrav
 * All rights reserved
 *
 * https://github.com/freebsd/freebsd/blob/master/sys/libkern/murmur3_32.c
 */
static uint32_t murmur3_32_hash(const void *data, size_t len, uint32_t seed)
{
        const uint8_t *bytes = (const uint8_t *)data;
        uint32_t hash = seed;
        uint32_t k = 0;
        size_t result = len;

        while (result >= 4) {
                k = le32dec(bytes);
                bytes += 4;
                result -= 4;
                k *= 0xcc9e2d51;
                k = ROL32(k, 15);
                k *= 0x1b873593;
                hash ^= k;
                hash = ROL32(hash, 13);
                hash *= 5;
                hash += 0xe6546b64;
        }

        if (result > 0) {
                k = 0;
                switch (result) {
                case 3:
                        k |= (uint32_t)bytes[2] << 16; // Fallthrough
                case 2:
                        k |= (uint32_t)bytes[1] << 8; // Fallthrough
                case 1:
                        k |= bytes[0];
                        k *= 0xcc9e2d51;
                        k = ROL32(k, 15);
                        k *= 0x1b873593;
                        hash ^= k;
                        break;
                }
        }

        // finalize the hash
        hash ^= (uint32_t)len;
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;

        return hash;
}

static ATTR_INLINE void *duplicate_key(const void *key, size_t key_size)
{
        void *new_key = malloc(key_size);

        if (new_key == NULL) {
                return NULL;
        }

        memcpy(new_key, key, key_size);

        return new_key;
}

static ATTR_INLINE void *duplicate_key_lower(const void *key, size_t key_size)
{
        char *new_key = (char *)duplicate_key(key, key_size);

        if (new_key == NULL) {
                return NULL;
        }

        for (size_t i = 0; i < key_size; ++i) {
                new_key[i] = (char)tolower(new_key[i]);
        }

        return new_key;
}

static ATTR_INLINE uint32_t get_power_two(uint32_t num)
{
        uint32_t i = 1;

        while (i < num) {
                i <<= 1;
        }

        return i;
}

static struct dict_table *create_map_internal(size_t size, uint8_t flags)
{
        struct dict_table *table = malloc(sizeof(struct dict_table));

        if (table == NULL) {
                return NULL;
        }

        table->bucket_count = get_power_two((uint32_t)size);
        table->entries_count = 0;
        table->entries = NULL;
#ifdef USE_POSIX
        if (pthread_mutex_init(&table->mutex, NULL) != 0) {
                return NULL;
        }
#endif
        table->flags = flags;
        table->hash_function = key_hash;
        table->itr_bucket_index = 0;
        table->high_load_factor = 0.75;
        table->low_load_factor = 0.20;
        table->free_function = NULL;
        table->event_flags = 0;

        return table;
}

struct dict_table *create_map(size_t size)
{
        return create_map_internal(size, DICT_TABLE_IGNORE_THRESHOLDS_EMPTY);
}

struct dict_table *create_map_with_flags(size_t size, uint8_t flags)
{
        return create_map_internal(size,
                                   flags | DICT_TABLE_IGNORE_THRESHOLDS_EMPTY);
}

uint32_t key_hash(const char *key)
{
        // TODO: Maybe grab a seed from env variables for startup
        return murmur3_32_hash((const uint8_t *)key, strlen(key), 1);
}

void map_set_flags(struct dict_table *table, uint8_t flags)
{
        table->flags = flags;
}
