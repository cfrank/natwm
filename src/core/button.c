// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <common/constants.h>
#include <common/logger.h>

#include "button.h"

static uint16_t *resolve_modifier_masks(const struct button_modifiers *modifiers)
{
        int index = -1;
        uint16_t *modifier_masks = malloc(sizeof(uint16_t) * 8);

        if (modifier_masks == NULL) {
                return NULL;
        }

        if (modifiers->num_lock != XCB_NO_SYMBOL && modifiers->caps_lock != XCB_NO_SYMBOL
            && modifiers->scroll_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index]
                        = (modifiers->num_lock | modifiers->caps_lock | modifiers->scroll_lock);
        }

        if (modifiers->num_lock != XCB_NO_SYMBOL && modifiers->caps_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index] = (modifiers->num_lock | modifiers->caps_lock);
        }

        if (modifiers->num_lock != XCB_NO_SYMBOL && modifiers->scroll_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index] = (modifiers->num_lock | modifiers->scroll_lock);
        }

        if (modifiers->caps_lock != XCB_NO_SYMBOL && modifiers->scroll_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index] = (modifiers->caps_lock | modifiers->scroll_lock);
        }

        if (modifiers->num_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index] = modifiers->num_lock;
        }

        if (modifiers->caps_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index] = modifiers->caps_lock;
        }

        if (modifiers->scroll_lock != XCB_NO_SYMBOL) {
                modifier_masks[++index] = modifiers->scroll_lock;
        }

        modifier_masks[++index] = XCB_NO_SYMBOL;

        return modifier_masks;
}

// Heavily influenced from bspwm:
// https://github.com/baskerville/bspwm/blob/master/src/pointer.c
//
// Given a xcb_keysym_t attempt to find and return the mod mask
static uint16_t mask_from_keysym(xcb_key_symbols_t *symbols, xcb_keycode_t *modifiers,
                                 uint8_t modifier_num, uint8_t keycodes_per_modifier,
                                 xcb_keysym_t keysym)
{
        xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(symbols, keysym);

        if (keycodes == NULL) {
                return 0;
        }

        for (uint8_t i = 0; i < modifier_num; ++i) {
                for (uint8_t j = 0; j < keycodes_per_modifier; ++j) {
                        xcb_keycode_t modifier = modifiers[i * keycodes_per_modifier + j];

                        if (modifier == XCB_NO_SYMBOL) {
                                continue;
                        }

                        for (xcb_keycode_t *key = keycodes; *key != XCB_NO_SYMBOL; key++) {
                                if (*key == modifier) {
                                        free(keycodes);

                                        return (uint16_t)(1 << i);
                                }
                        }
                }
        }

        free(keycodes);

        return 0;
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

        int modifier_num
                = xcb_get_modifier_mapping_keycodes_length(reply) / reply->keycodes_per_modifier;

        // Usually (Mod1-Mod5, Shift, Control, Lock)
        assert(modifier_num >= 0 && modifier_num <= UINT8_MAX);

        modifiers->num_lock = mask_from_keysym(symbols,
                                               modifier_keycodes,
                                               (uint8_t)modifier_num,
                                               reply->keycodes_per_modifier,
                                               NUM_LOCK_KEYSYM);
        modifiers->caps_lock = mask_from_keysym(symbols,
                                                modifier_keycodes,
                                                (uint8_t)modifier_num,
                                                reply->keycodes_per_modifier,
                                                CAPS_LOCK_KEYSYM);
        modifiers->scroll_lock = mask_from_keysym(symbols,
                                                  modifier_keycodes,
                                                  (uint8_t)modifier_num,
                                                  reply->keycodes_per_modifier,
                                                  SCROLL_LOCK_KEYSYM);

        if (modifiers->caps_lock == XCB_NO_SYMBOL) {
                modifiers->caps_lock = XCB_MOD_MASK_LOCK;
        }

        modifiers->modifier_masks = resolve_modifier_masks(modifiers);

        if (modifiers->modifier_masks == NULL) {
                free(modifiers);

                xcb_key_symbols_free(symbols);

                free(reply);

                return NULL;
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

ATTR_INLINE uint16_t button_modifiers_get_clean_mask(const struct button_modifiers *modifiers,
                                                     uint16_t mask)
{
        uint16_t modifier_mask
                = (modifiers->num_lock | modifiers->caps_lock | modifiers->scroll_lock);

        return (uint16_t)(mask & ~(modifier_mask));
}

ATTR_INLINE void button_binding_grab(const struct natwm_state *state, xcb_window_t window,
                                     const struct button_binding *binding)
{
        xcb_grab_button(state->xcb,
                        binding->pass_event,
                        window,
                        binding->mask,
                        binding->pointer_mode,
                        binding->keyboard_mode,
                        XCB_NONE,
                        binding->cursor,
                        binding->button,
                        binding->modifiers);

        struct button_modifiers *modifiers = state->button_state->modifiers;

        if (modifiers == NULL) {
                return;
        }

        for (uint16_t *mask = modifiers->modifier_masks; *mask != XCB_NO_SYMBOL; mask++) {
                xcb_grab_button(state->xcb,
                                binding->pass_event,
                                window,
                                binding->mask,
                                binding->pointer_mode,
                                binding->keyboard_mode,
                                XCB_NONE,
                                binding->cursor,
                                binding->button,
                                binding->modifiers | *mask);
        }
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

                button_binding_grab(state, client->window, &binding);
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
                free(state->modifiers->modifier_masks);
                free(state->modifiers);
        }

        free(state);
}
