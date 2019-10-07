// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stddef.h>
#include <stdint.h>

uint32_t hash_murmur3_32(const void *data, size_t len, uint32_t seed);
