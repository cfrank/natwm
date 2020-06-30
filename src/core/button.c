// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <common/logger.h>

#include "button.h"

// Heavily influenced from bspwm:
// https://github.com/baskerville/bspwm/blob/master/src/pointer.c
//
// Given a xcb_keysym_t attempt to find and return the mod mask
static uint16_t mask_from_keysym(xcb_key_symbols_t *symbols, xcb_keycode_t *modifiers,
                                 uint16_t modifier_length, uint8_t keycodes_per_modifier,
                                 xcb_keysym_t keysym)
{
        uint16_t result = 0;
        xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(symbols, keysym);

        if (keycodes == NULL) {
                return result;
        }

        for (uint16_t i = 0; i < modifier_length; ++i) {
                for (uint8_t j = 0; j < keycodes_per_modifier; ++j) {
                        xcb_keycode_t modifier = modifiers[i * keycodes_per_modifier + j];

                        if (modifier == XCB_NO_SYMBOL) {
                                continue;
                        }

                        for (xcb_keycode_t *key = keycodes; *key != XCB_NO_SYMBOL; key++) {
                                if (*key == modifier) {
                                        result |= (uint16_t)(1 << i);
                                }
                        }
                }
        }

        free(keycodes);

        return result;
}

// Heavily influenced from bspwm:
// https://github.com/baskerville/bspwm/blob/master/src/pointer.c
//
// We need to find the modifier mask for our modifier keys. This will allow us to correctly set
// up button handlers in the future.
//
// An example is when you are trying to focus on a window with caps lock active. This no longer
// resolves as just a simple XCB_BUTTON_INDEX_1 event it is a XCB_BUTTON_INDEX_1 event with
// the additional CAPS_LOCK modifier active.
static struct button_modifiers *resolve_button_modifiers(xcb_connection_t *connection)
{
        struct button_modifiers *modifiers = malloc(sizeof(struct button_modifiers));

        if (modifiers == NULL) {
                return NULL;
        }

        xcb_key_symbols_t *symbols = xcb_key_symbols_alloc(connection);

        if (symbols == NULL) {
                free(modifiers);

                return NULL;
        }

        xcb_get_modifier_mapping_cookie_t cookie = xcb_get_modifier_mapping(connection);
        xcb_get_modifier_mapping_reply_t *reply
                = xcb_get_modifier_mapping_reply(connection, cookie, NULL);

        if (reply == NULL || reply->keycodes_per_modifier < 1) {
                free(modifiers);

                xcb_key_symbols_free(symbols);

                return NULL;
        }

        xcb_keycode_t *modifier_keycodes = xcb_get_modifier_mapping_keycodes(reply);

        if (modifier_keycodes == NULL) {
                free(modifiers);

                xcb_key_symbols_free(symbols);

                free(reply);

                return NULL;
        }

        int modifier_length
                = xcb_get_modifier_mapping_keycodes_length(reply) / reply->keycodes_per_modifier;

        assert(modifier_length >= 0 && modifier_length <= UINT16_MAX);

        modifiers->num_lock = mask_from_keysym(symbols,
                                               modifier_keycodes,
                                               (uint16_t)modifier_length,
                                               reply->keycodes_per_modifier,
                                               NUM_LOCK_KEYSYM);
        modifiers->caps_lock = mask_from_keysym(symbols,
                                                modifier_keycodes,
                                                (uint16_t)modifier_length,
                                                reply->keycodes_per_modifier,
                                                CAPS_LOCK_KEYSYM);
        modifiers->scroll_lock = mask_from_keysym(symbols,
                                                  modifier_keycodes,
                                                  (uint16_t)modifier_length,
                                                  reply->keycodes_per_modifier,
                                                  SCROLL_LOCK_KEYSYM);

        if (modifiers->caps_lock == XCB_NO_SYMBOL) {
                modifiers->caps_lock = XCB_MOD_MASK_LOCK;
        }

        xcb_key_symbols_free(symbols);
        free(reply);

        return modifiers;
}

struct button_state *button_state_create(xcb_connection_t *connection)
{
        struct button_state *state = malloc(sizeof(struct button_state));

        if (state == NULL) {
                return NULL;
        }

        state->modifiers = resolve_button_modifiers(connection);

        if (state->modifiers == NULL) {
                LOG_WARNING(natwm_logger, "Failed to resolve modifier keys! This may cause issues");
        }

        state->grabbed_client = NULL;

        return state;
}

void button_event_grab(xcb_connection_t *connection, xcb_window_t window,
                       const struct button_binding *binding)
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
void button_initialize_client_listeners(const struct natwm_state *state,
                                        const struct client *client)
{
        for (size_t i = 0; i < BUTTON_EVENTS_NUM; ++i) {
                struct button_binding binding = button_events[i];

                button_event_grab(state->xcb, client->window, &binding);
        };

        xcb_flush(state->xcb);
}

enum natwm_error button_handle_focus(struct natwm_state *state, struct workspace *workspace,
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

void button_state_destroy(struct button_state *state)
{
        if (state->modifiers != NULL) {
                free(state->modifiers);
        }

        free(state);
}
