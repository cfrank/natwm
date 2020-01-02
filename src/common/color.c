// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>
#include <string.h>

#include "color.h"

struct color_value *color_value_from_string(const char *string)
{
        struct color_value *value = malloc(sizeof(struct color_value));

        if (value == NULL) {
                return NULL;
        }

        value->string = string;

        return value;
}

bool color_value_has_changed(struct color_value *value,
                             const char *new_string_value)
{
        if (strcmp(value->string, new_string_value) == 0) {
                return true;
        }

        return false;
}

void color_value_destroy(struct color_value *value)
{
        free(value);
}
