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
