// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <clog.h>
#include <stdbool.h>

extern struct logger *natwm_logger;

void initialize_logger(bool verbose);
