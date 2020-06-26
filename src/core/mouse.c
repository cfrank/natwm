// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/xcb.h>

#include "mouse.h"

void mouse_event_grab_button(xcb_connection_t *connection, xcb_window_t window,
                             const struct mouse_binding *binding)
{
        xcb_grab_button(connection,
                        binding->pass_event,
                        window,
                        binding->mask,
                        binding->pointer_mode,
                        binding->keyboard_mode,
                        XCB_NONE,
                        binding->cursor,
                        binding->button,
                        binding->modifiers);
}

// These are the mouse events which will persist for the life of the client.
// The only mouse event which will not be initialized here is the "click to
// focus" event which needs to be created or destroyed based on the client
// focus
void mouse_initialize_client_listeners(const struct natwm_state *state, const struct client *client)
{
        for (size_t i = 0; i < MOUSE_EVENTS_NUM; ++i) {
                struct mouse_binding binding = mouse_events[i];

                mouse_event_grab_button(state->xcb, client->window, &binding);
        };

        xcb_flush(state->xcb);
}

enum natwm_error mouse_handle_focus(struct natwm_state *state, struct workspace *workspace,
                                    struct client *client)
{
        enum natwm_error err = workspace_focus_client(state, workspace, client);

        if (err != NO_ERROR) {
                return err;
        }

        // For the focus event we queue the event, and once we have
        // focused both the workspace (if needed) and the client we
        // release the queued event and the client receives the event
        // like normal
        xcb_allow_events(state->xcb, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);

        return NO_ERROR;
}
