// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "dict.h"

/**
 * Use perl's hashing function by default
 *
 * Customizing is provided by the hash_function property on dict_table
 */
static unsigned int hash_function(const void *key, size_t key_size)
{
        register size_t i = key_size;
        register const unsigned char *s = (const unsigned char *)key;
        register unsigned int ret = 0;

        while (i--) {
                ret += *s++;
                ret += (ret << 10);
                ret ^= (ret >> 6);
        }

        ret += (ret << 3);
        ret ^= (ret >> 11);
        ret += (ret << 15);

        return ret;
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
