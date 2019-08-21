// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <common/hash.h>
#include "map.h"

static int entry_is_present(struct dict_entry *entry)
{
        return entry->data != NULL;
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
        map->hash_function = hash_murmur3_32;
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
                free(map->entries[i]->data);
                free(map->entries[i]);
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
                free_function(map->entries[i]->data);
                free(map->entries[i]);
        }

        free(map);
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
