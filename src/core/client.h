// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

enum client_type {
        DOCK,
        TOOLBAR,
        SPLASH,
        NORMAL,
};

struct client {
        uint32_t workspace_index;
        xcb_rectangle_t rect;
        xcb_rectangle_t floating_rect;
        enum client_type type;
        bool is_sticky;
        bool is_fullscreen;
        bool is_urgent;
};
