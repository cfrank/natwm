// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <stdlib.h>

#include <common/logger.h>

#include "workspace.h"

struct workspace *workspace_create(xcb_rectangle_t *rects, size_t count)
{
        struct workspace *workspace = malloc(sizeof(struct workspace));

        if (workspace == NULL) {
                return NULL;
        }

        workspace->length = count;
        workspace->focused_space = NULL;
        workspace->spaces = create_list();

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

                list_insert(workspace->spaces, space);
        }

        free(rects);

        return workspace;
}

enum natwm_error workspace_set_focused_space(struct workspace *workspace,
                                             size_t index)
{
        struct list *spaces = workspace->spaces;

        for (struct node *node = spaces->head; node != NULL;
             node = node->next) {
                struct space *space = (struct space *)node->data;

                if (space->index == index) {
                        workspace->focused_space = space;
                        space->is_focused = true;
                }

                return NO_ERROR;
        }

        return NOT_FOUND_ERROR;
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
        space->clients = create_list();

        if (space->clients == NULL) {
                free(space);

                return NULL;
        }

        return space;
}

void workspace_destroy(struct workspace *workspace)
{
        for (struct node *space = workspace->spaces->head; space != NULL;
             space = space->next) {
                space_destroy((struct space *)space->data);
        }

        destroy_list(workspace->spaces);
        free(workspace);
}

void space_destroy(struct space *space)
{
        for (struct node *client = space->clients->head; client != NULL;
             client = client->next) {
                LOG_INFO(natwm_logger, "TODO: Free client");
        }

        destroy_list(space->clients);

        free(space);
}
