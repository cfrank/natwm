// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stddef.h>
#include <stdlib.h>

#include "list.h"

struct list *create_list(void)
{
        struct list *list = malloc(sizeof(struct list));

        if (list == NULL) {
                return NULL;
        }

        list->head = NULL;
        list->tail = NULL;
        list->size = 0;

        return list;
}
