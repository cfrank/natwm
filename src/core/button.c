// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <assert.h>
#include <stdlib.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_keysyms.h>

#include <common/constants.h>
#include <common/logger.h>

#include "button.h"

// Toggleable keys
// https://cgit.freedesktop.org/xorg/proto/x11proto/tree/keysymdef.h
#define NUM_LOCK_KEYSYM 0xff7f
#define CAPS_LOCK_KEYSYM 0xffe5
#define SCROLL_LOCK_KEYSYM 0xff14

#ifdef __APPLE__
#define MODIFIER_COUNT_FALLBACK 8
#endif

// Return a list of all the modifier masks we need to account for in our calls to
// xcb_grab_button
static uint16_t *resolve_toggle_masks(const struct toggle_modifiers *modifiers)
{
        uint16_t *masks = malloc(sizeof(uint16_t) * 8);

        if (masks == NULL) {
                return NULL;
        }

        int index = -1;

        if (modifiers->num_lock != XCB_NONE && modifiers->caps_lock != XCB_NONE
            && modifiers->scroll_lock != XCB_NONE) {
                masks[++index]
                        = (modifiers->num_lock | modifiers->caps_lock | modifiers->scroll_lock);
        }

        if (modifiers->num_lock != XCB_NONE && modifiers->caps_lock != XCB_NONE) {
                masks[++index] = (modifiers->num_lock | modifiers->caps_lock);
        }

        if (modifiers->num_lock != XCB_NONE && modifiers->scroll_lock != XCB_NONE) {
                masks[++index] = (modifiers->num_lock | modifiers->scroll_lock);
        }

        if (modifiers->caps_lock != XCB_NONE && modifiers->scroll_lock != XCB_NONE) {
                masks[++index] = (modifiers->caps_lock | modifiers->scroll_lock);
        }

        if (modifiers->num_lock != XCB_NONE) {
                masks[++index] = modifiers->num_lock;
        }

        if (modifiers->caps_lock != XCB_NONE) {
                masks[++index] = modifiers->caps_lock;
        }

        if (modifiers->scroll_lock != XCB_NONE) {
                masks[++index] = modifiers->scroll_lock;
        }

        masks[++index] = XCB_NONE;

        return masks;
}

// Heavily influenced from bspwm:
// https://github.com/baskerville/bspwm/blob/master/src/pointer.c
//
// Given a xcb_keysym_t attempt to find and return the mod mask
static uint16_t modifier_mask_from_keysym(xcb_key_symbols_t *symbols,
                                          const xcb_keycode_t *modifiers, uint8_t modifier_count,
                                          uint8_t keycodes_per_modifier, xcb_keysym_t keysym)
{
        xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(symbols, keysym);

        if (keycodes == NULL) {
                return 0;
        }

        for (uint8_t i = 0; i < modifier_count; ++i) {
                for (uint8_t j = 0; j < keycodes_per_modifier; ++j) {
                        xcb_keycode_t modifier = modifiers[i * keycodes_per_modifier + j];

                        if (modifier == XCB_NONE) {
                                continue;
                        }

                        for (xcb_keycode_t *key = keycodes; *key != XCB_NONE; key++) {
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
// resolves as just a simple XCB_BUTTON_INDEX_1 event since it is now a XCB_BUTTON_INDEX_1 event
// with the additional CAPS_LOCK modifier active.
static struct toggle_modifiers *resolve_toggle_modifiers(xcb_connection_t *connection)
{
        struct toggle_modifiers *modifiers = malloc(sizeof(struct toggle_modifiers));

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

#ifdef __APPLE__
        if (reply == NULL) {
#else
        if (reply == NULL || reply->keycodes_per_modifier < 1) {
#endif
                free(modifiers);

                xcb_key_symbols_free(symbols);

                return NULL;
        }

        const xcb_keycode_t *modifier_keycodes = xcb_get_modifier_mapping_keycodes(reply);

        if (modifier_keycodes == NULL) {
                goto handle_error;
        }

#ifdef __APPLE__
        // On OSX keycodes_per_modifier returns 0 for some reason.
        //
        // TODO: Determine if this is correct
        int modifier_count = MODIFIER_COUNT_FALLBACK;
#else
        int modifier_count
                = xcb_get_modifier_mapping_keycodes_length(reply) / reply->keycodes_per_modifier;

        // Usually (Mod1-Mod5, Shift, Control, Lock)
        assert(modifier_count >= 0 && modifier_count <= UINT8_MAX);
#endif

        modifiers->num_lock = modifier_mask_from_keysym(symbols,
                                                        modifier_keycodes,
                                                        (uint8_t)modifier_count,
                                                        reply->keycodes_per_modifier,
                                                        NUM_LOCK_KEYSYM);
        modifiers->caps_lock = modifier_mask_from_keysym(symbols,
                                                         modifier_keycodes,
                                                         (uint8_t)modifier_count,
                                                         reply->keycodes_per_modifier,
                                                         CAPS_LOCK_KEYSYM);
        modifiers->scroll_lock = modifier_mask_from_keysym(symbols,
                                                           modifier_keycodes,
                                                           (uint8_t)modifier_count,
                                                           reply->keycodes_per_modifier,
                                                           SCROLL_LOCK_KEYSYM);

        if (modifiers->caps_lock == XCB_NONE) {
                modifiers->caps_lock = XCB_MOD_MASK_LOCK;
        }

        modifiers->masks = resolve_toggle_masks(modifiers);

        if (modifiers->masks == NULL) {
                goto handle_error;
        }

        xcb_key_symbols_free(symbols);
        free(reply);

        return modifiers;

handle_error:
        free(modifiers);

        xcb_key_symbols_free(symbols);

        free(reply);

        return NULL;
}

// When resizing we will show a dummy window which will represent the desired size of the window.
// When handling ungrab events we will use the size of this dummy window to configure the new size
// of the client
static xcb_window_t create_resize_helper(const struct natwm_state *state)
{
        xcb_window_t resize_helper = xcb_generate_id(state->xcb);
        const struct theme *theme = state->workspace_list->theme;
        uint16_t mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL;
        uint32_t values[] = {
                theme->resize_background_color->color_value,
                theme->resize_border_color->color_value,
        };

        xcb_create_window(state->xcb,
                          XCB_COPY_FROM_PARENT,
                          resize_helper,
                          state->screen->root,
                          -1,
                          -1,
                          1,
                          1,
                          0,
                          XCB_COPY_FROM_PARENT,
                          state->screen->root_visual,
                          mask,
                          values);

        xcb_icccm_set_wm_class(state->xcb,
                               resize_helper,
                               sizeof(RESIZE_HELPER_WINDOW_CLASS_NAME),
                               RESIZE_HELPER_WINDOW_CLASS_NAME);

        xcb_map_window(state->xcb, resize_helper);

        xcb_flush(state->xcb);

        return resize_helper;
}

static void initialize_resize_helper(struct natwm_state *state, const xcb_rectangle_t *monitor_rect,
                                     const struct client *client)
{
        // On first resize the resize helper will not have been created yet
        if (state->button_state->resize_helper == XCB_NONE) {
                natwm_state_lock(state);

                state->button_state->resize_helper = create_resize_helper(state);

                natwm_state_unlock(state);
        }

        uint16_t border_width
                = client_get_active_border_width(state->workspace_list->theme, client);
        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
                | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH
                | XCB_CONFIG_WINDOW_STACK_MODE;
        uint32_t values[] = {
                (uint32_t)(client->rect.x + monitor_rect->x),
                (uint32_t)(client->rect.y + monitor_rect->y),
                client->rect.width,
                client->rect.height,
                border_width,
                XCB_STACK_MODE_ABOVE,
        };

        xcb_configure_window(state->xcb, state->button_state->resize_helper, mask, values);

        xcb_flush(state->xcb);
}

static void resize_helper(const struct natwm_state *state, int16_t offset_x, int16_t offset_y)
{
        if (state->button_state->resize_helper == XCB_NONE) {
                return;
        }

        uint16_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        uint32_t values[] = {
                (uint32_t)(state->button_state->grabbed_client->rect.width + offset_x),
                (uint32_t)(state->button_state->grabbed_client->rect.height + offset_y),
        };

        xcb_configure_window(state->xcb, state->button_state->resize_helper, mask, values);
}

static void hide_resize_helper(const struct natwm_state *state)
{
        if (state->button_state->resize_helper == XCB_NONE) {
                return;
        }

        uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH
                | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_BORDER_WIDTH
                | XCB_CONFIG_WINDOW_STACK_MODE;
        int32_t values[] = {
                -1,
                -1,
                1,
                1,
                0,
                XCB_STACK_MODE_BELOW,
        };

        xcb_configure_window(state->xcb, state->button_state->resize_helper, mask, values);

        xcb_flush(state->xcb);
}

static void button_state_reset(struct natwm_state *state)
{
        natwm_state_lock(state);

        state->button_state->grabbed_client = NULL;
        state->button_state->monitor_rect = NULL;
        state->button_state->start_x = 0;
        state->button_state->start_y = 0;

        natwm_state_unlock(state);
}

struct button_state *button_state_create(xcb_connection_t *connection)
{
        struct button_state *state = malloc(sizeof(struct button_state));

        if (state == NULL) {
                return NULL;
        }

        state->modifiers = resolve_toggle_modifiers(connection);

        if (state->modifiers == NULL) {
                LOG_WARNING(natwm_logger,
                            "Failed to resolve toggleable modifier keys! This may cause issues");
        }

        state->grabbed_client = NULL;
        state->resize_helper = XCB_NONE;

        return state;
}

ATTR_INLINE uint16_t toggle_modifiers_get_clean_mask(const struct toggle_modifiers *modifiers,
                                                     uint16_t mask)
{
        if (modifiers == NULL) {
                // Use simple clean mask if we were unable to resolve modifiers
                return (uint16_t)(mask & ~(XCB_MOD_MASK_LOCK));
        }

        uint16_t modifier_mask
                = (modifiers->num_lock | modifiers->caps_lock | modifiers->scroll_lock);

        return (uint16_t)(mask & ~(modifier_mask));
}

void button_binding_grab(const struct natwm_state *state, xcb_window_t window,
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

        struct toggle_modifiers *modifiers = state->button_state->modifiers;

        if (modifiers == NULL) {
                return;
        }

        for (uint16_t *mask = modifiers->masks; *mask != XCB_NONE; mask++) {
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

void button_binding_ungrab(const struct natwm_state *state, xcb_window_t window,
                           const struct button_binding *binding)
{
        xcb_ungrab_button(state->xcb, binding->button, window, binding->modifiers);

        struct toggle_modifiers *modifiers = state->button_state->modifiers;

        if (modifiers == NULL) {
                return;
        }

        for (uint16_t *mask = modifiers->masks; *mask != XCB_NONE; ++mask) {
                xcb_ungrab_button(state->xcb, binding->button, window, binding->modifiers | *mask);
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

enum natwm_error button_handle_grab(struct natwm_state *state,
                                    const xcb_button_press_event_t *event,
                                    xcb_rectangle_t *monitor_rect, struct client *client)
{
        if (!event->same_screen) {
                LOG_ERROR(natwm_logger,
                          "Receieved a grab event which did not occur on the root window");

                return INVALID_INPUT_ERROR;
        }

        if (state->button_state->grabbed_client != NULL) {
                LOG_ERROR(natwm_logger, "Attempting to grab a second client");

                return RESOLUTION_FAILURE;
        }

        if (client->is_fullscreen) {
                // Ignore clients if they are currently fullscreen
                return NO_ERROR;
        }

        natwm_state_lock(state);

        state->button_state->grabbed_client = client;
        state->button_state->monitor_rect = monitor_rect;
        state->button_state->start_x = event->event_x;
        state->button_state->start_y = event->event_y;

        natwm_state_unlock(state);

        if (event->detail == XCB_BUTTON_INDEX_3) {
                initialize_resize_helper(state, monitor_rect, client);
        }

        return NO_ERROR;
}

enum natwm_error button_handle_motion(struct natwm_state *state, uint16_t mouse_mask, int16_t x,
                                      int16_t y)
{
        if (state->button_state->grabbed_client == NULL) {
                LOG_ERROR(natwm_logger,
                          "Received motion event when there isn't a currently grabbed client");

                return INVALID_INPUT_ERROR;
        }

        if (state->button_state->grabbed_client->is_fullscreen) {
                // Ignore full screen clients
                return NO_ERROR;
        }

        int16_t offset_x = (int16_t)(x - state->button_state->start_x);
        int16_t offset_y = (int16_t)(y - state->button_state->start_y);

        if (mouse_mask & XCB_BUTTON_MASK_1) {
                return client_handle_drag(
                        state, state->button_state->grabbed_client, offset_x, offset_y);
        }

        if (mouse_mask & XCB_BUTTON_MASK_3) {
                resize_helper(state, offset_x, offset_y);
        }

        return NO_ERROR;
}

void button_handle_drag_end(struct natwm_state *state)
{
        if (state->button_state->grabbed_client == NULL) {
                return;
        }

        hide_resize_helper(state);
        button_state_reset(state);
}

enum natwm_error button_handle_resize_end(struct natwm_state *state,
                                          const xcb_button_release_event_t *event)
{
        if (state->button_state->grabbed_client == NULL) {
                return NO_ERROR;
        }

        int16_t offset_x = (int16_t)(event->event_x - state->button_state->start_x);
        int16_t offset_y = (int16_t)(event->event_y - state->button_state->start_y);

        enum natwm_error err = client_handle_resize(
                state, state->button_state->grabbed_client, offset_x, offset_y);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger, "Failed to perform resize");

                return err;
        }

        hide_resize_helper(state);
        button_state_reset(state);

        return NO_ERROR;
}

void button_state_destroy(struct natwm_state *state)
{
        if (state->button_state->modifiers != NULL) {
                free(state->button_state->modifiers->masks);
                free(state->button_state->modifiers);
        }

        if (state->button_state->resize_helper != XCB_NONE) {
                xcb_unmap_window(state->xcb, state->button_state->resize_helper);
        }

        free(state->button_state);
}
