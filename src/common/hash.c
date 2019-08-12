// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include "hash.h"
#include "constants.h"

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
uint32_t hash_murmur3_32(const void *data, size_t len, uint32_t seed)
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

