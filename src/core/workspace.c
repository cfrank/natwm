// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include "workspace.h"

struct workspace *workspace_create(xcb_rectangle_t *rects, size_t count)
{
        struct workspace *workspace = malloc(sizeof(struct workspace));

        if (workspace == NULL) {
                return NULL;
        }

        workspace->length = count;
        workspace->spaces = malloc(sizeof(struct space) * count);

        if (workspace->spaces == NULL) {
                free(workspace);

                return NULL;
        }

        for (size_t i = 0; i < count; ++i) {
                struct space *space = space_create(rects[i], i);

                if (space == NULL) {
                        workspace_destroy(workspace);

                        return NULL;
                }

                workspace->spaces[i] = space;
        }

        return workspace;
}

struct space *space_create(xcb_rectangle_t rect, size_t index)
{
        struct space *space = malloc(sizeof(struct space));

        if (space == NULL) {
                return NULL;
        }

        space->index = index;
        space->rect = rect;
        space->is_visible = false;
        space->is_focused = false;
        space->is_floating = false;

        return space;
}

void workspace_destroy(struct workspace *workspace)
{
        for (size_t i = 0; i < workspace->length; ++i) {
                if (workspace->spaces[i] != NULL) {
                        space_destroy(workspace->spaces[i]);
                }
        }

        free(workspace);
}

void space_destroy(struct space *space)
{
        free(space);
}

