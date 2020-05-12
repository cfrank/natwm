// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <xcb/xcb.h>

#include "mouse.h"

void mouse_client_listener(const struct natwm_state *state,
                           const struct client *client)
{
        for (size_t i = 0; i < MOUSE_EVENTS_NUM; ++i) {
                struct mouse_binding binding = mouse_events[i];

                xcb_grab_button(state->xcb,
                                binding.pass_event,
                                client->window,
                                binding.mask,
                                binding.pointer_mode,
                                binding.keyboard_mode,
                                XCB_NONE,
                                binding.cursor,
                                binding.button,
                                binding.modifiers);
        };

        xcb_flush(state->xcb);
}
