// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_util.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/map.h>
#include <common/theme.h>
#include <common/util.h>
#include <core/client.h>
#include <core/config/config.h>
#include <core/events/event.h>
#include <core/ewmh.h>
#include <core/monitor.h>
#include <core/state.h>
#include <core/workspace.h>

struct argument_options {
        const char *config_path;
        const char *screen;
        bool verbose;
};

enum status {
        STOPPED = 1U << 0U,
        RUNNING = 1U << 1U,
};

static enum status program_status = STOPPED;

static void handle_connection_error(int error)
{
        const char *message = NULL;

        switch (error) {
        case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
                message = "Connection to the X Server failed: Extension not "
                          "supported";
                break;
        case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
                message = "Connection to the X server failed: Lack of memory";
                break;
        case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
                message = "Connection to the X server failed: Invalid request";
                break;
        case XCB_CONN_CLOSED_PARSE_ERR:
                message = "Connection to the X server failed: Invalid display";
                break;
        case XCB_CONN_CLOSED_INVALID_SCREEN:
                message = "Connection to the X server failed: Screen not found";
                break;
        default:
                message = "Connection to the X server failed";
        }

        LOG_CRITICAL(natwm_logger, message);
}

static xcb_connection_t *make_connection(const char *screen, int *screen_num)
{
        xcb_connection_t *connection = xcb_connect(screen, screen_num);

        int connection_error = xcb_connection_has_error(connection);

        if (connection_error > 0) {
                handle_connection_error(connection_error);

                // Still need to free connection
                xcb_disconnect(connection);

                return NULL;
        }

        return connection;
}

static xcb_screen_t *find_default_screen(xcb_connection_t *connection,
                                         int screen_num)
{
        const xcb_setup_t *setup = xcb_get_setup(connection);
        xcb_screen_iterator_t itr = xcb_setup_roots_iterator(setup);

        for (int i = screen_num; itr.rem; --i, xcb_screen_next(&itr)) {
                if (i == 0) {
                        return itr.data;
                }
        }

        return NULL;
}

static void signal_handler(int signum)
{
        UNUSED_FUNCTION_PARAM(signum);

        program_status = STOPPED;
}

static int install_signal_handlers(void)
{
        struct sigaction action;

        action.sa_handler = &signal_handler;

        sigemptyset(&action.sa_mask);

        sigaddset(&action.sa_mask, SIGTERM);
        sigaddset(&action.sa_mask, SIGINT);
        sigaddset(&action.sa_mask, SIGHUP);

        action.sa_flags = SA_RESTART;

        // Handle the following actions
        if (sigaction(SIGTERM, &action, NULL) == -1
            || sigaction(SIGINT, &action, NULL) == -1
            || sigaction(SIGHUP, &action, NULL) == -1) {
                return -1;
        }

        return 0;
}

// In order to operate we need to subscribe to events on the root window.
//
// The event masks placed on the root window will provide us with events which
// occur on our child windows
static int root_window_subscribe(const struct natwm_state *state)
{
        xcb_event_mask_t root_mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        xcb_void_cookie_t cookie = xcb_change_window_attributes_checked(
                state->xcb, state->screen->root, XCB_CW_EVENT_MASK, &root_mask);
        xcb_generic_error_t *error = xcb_request_check(state->xcb, cookie);

        xcb_flush(state->xcb);

        if (error != NULL) {
                // We will fail if there is already a window manager present
                free(error);

                return -1;
        }

        return 0;
}

static void *start_wm_events_thread(void *passed_state)
{
        struct natwm_state *state = (struct natwm_state *)passed_state;

        while (program_status & RUNNING) {
                xcb_generic_event_t *event = xcb_wait_for_event(state->xcb);

                if (event) {
                        enum natwm_error err = event_handle(state, event);

                        if (err == NOT_FOUND_ERROR) {
                                free(event);

                                continue;
                        }

                        if (err != NO_ERROR) {
                                uint8_t rtype = event->response_type;
                                uint8_t type = (uint8_t)(GET_EVENT_TYPE(rtype));

                                LOG_WARNING(natwm_logger,
                                            "Failed to perform %s",
                                            xcb_event_get_label(type));

                                free(event);

                                continue;
                        }

                        free(event);

                        xcb_flush(state->xcb);

                        continue;
                }

                if (xcb_connection_has_error(state->xcb)
                    && program_status & RUNNING) {
                        LOG_ERROR(natwm_logger,
                                  "Connection to X server closed");

                        program_status = STOPPED;
                }
        }

        // Event loop stopped disconnect from x
        LOG_INFO(natwm_logger, "Disconnected...");

        return (void *)0;
}

static struct argument_options *parse_arguments(int argc, char **argv)
{
        int opt = 0;
        struct argument_options *arg_options
                = malloc(sizeof(struct argument_options));

        if (arg_options == NULL) {
                return NULL;
        }

        // defaults
        arg_options->config_path = NULL;
        arg_options->screen = NULL;
        arg_options->verbose = false;

        // disable default error handling behavior in getopt
        opterr = 0;

        while ((opt = getopt(argc, argv, "c:hs:vV")) != -1) {
                switch (opt) {
                case 'c':
                        arg_options->config_path = optarg;
                        break;
                case 'h':
                        printf("%s\n", NATWM_VERSION_STRING);
                        printf("-c <file>, Set the config file\n");
                        printf("-h,        Print this help message\n");
                        printf("-s,        Specify specific screen for X\n");
                        printf("-v,        Print version information\n");
                        printf("-V,        Verbose mode\n");

                        goto exit_success;
                case 's':
                        arg_options->screen = optarg;
                        break;
                case 'v':
                        printf("%s\n", NATWM_VERSION_STRING);
                        printf("Copywrite (c) 2019 Chris Frank\n");
                        printf("Released under the Revised BSD License\n");

                        goto exit_success;
                case 'V':
                        arg_options->verbose = true;
                        break;
                default:
                        // Handle invalid opt
                        fprintf(stderr,
                                "Recieved invalid command line argument '%c'\n",
                                optopt);

                        free(arg_options);

                        exit(EXIT_FAILURE);
                }
        }

        return arg_options;

exit_success:
        free(arg_options);

        exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
        int screen_num = 0;
        struct argument_options *arg_options = parse_arguments(argc, argv);

        if (arg_options == NULL) {
                fprintf(stderr, "Failed to parse command line arguments");

                exit(EXIT_FAILURE);
        }

        // Initialize the logger
        initialize_logger(arg_options->verbose);

        // Initialize program state
        struct natwm_state *state = natwm_state_create();

        if (state == NULL) {
                LOG_CRITICAL(natwm_logger,
                             "Failed to initialize applicaiton state");

                exit(EXIT_FAILURE);
        }

        state->screen_num = screen_num;

        // Initialize config
        if (arg_options->config_path) {
                state->config_path = arg_options->config_path;
        }

        const struct map *config = config_initialize_path(state->config_path);

        if (config == NULL) {
                goto free_and_error;
        }

        state->config = config;

        // Catch and handle signals
        if (install_signal_handlers() != 0) {
                LOG_ERROR(
                        natwm_logger,
                        "Failed to handle signals - This may cause problems!");
        }

        // Initialize x
        xcb_connection_t *xcb
                = make_connection(arg_options->screen, &screen_num);

        if (xcb == NULL) {
                goto free_and_error;
        }

        state->xcb = xcb;

        LOG_INFO(natwm_logger, "Successfully connected to X server");

        // Find the default screen
        xcb_screen_t *default_screen
                = find_default_screen(state->xcb, screen_num);

        if (default_screen == NULL) {
                LOG_ERROR(natwm_logger, "Failed to find default screen");

                goto free_and_error;
        }

        state->screen = default_screen;

        // Attempt to register for substructure events
        if (root_window_subscribe(state) != 0) {
                LOG_ERROR(natwm_logger,
                          "Failed to subscribe to root events: Other window "
                          "manager is present");

                goto free_and_error;
        }

        xcb_ewmh_connection_t *ewmh = ewmh_create(xcb);

        if (ewmh == NULL) {
                goto free_and_error;
        }

        state->ewmh = ewmh;

        // Initialize ewmh hinting
        ewmh_init(state);

        struct monitor_list *monitor_list = NULL;

        if (monitor_setup(state, &monitor_list) != NO_ERROR) {
                goto free_and_error;
        }

        state->monitor_list = monitor_list;

        struct workspace_list *workspace_list = NULL;

        if (workspace_list_init(state, &workspace_list) != NO_ERROR) {
                LOG_ERROR(natwm_logger, "Failed to setup workspaces");

                goto free_and_error;
        }

        state->workspace_list = workspace_list;

        // Before we can start registering clients we need to load the theme
        // from the configuration file. This will save us trips to the config
        // map when registering clients.
        //
        // TODO: Set defaults so we don't have a hard requirement on the user
        // settings these in their configuration
        state->workspace_list->theme = theme_create(state->config);

        if (state->workspace_list->theme == NULL) {
                LOG_INFO(natwm_logger, "Failed to set up theme");
                goto free_and_error;
        }

        program_status = RUNNING;

        // Start wm thread
        void *wm_events_result = NULL;
        pthread_t wm_events_thread;

        pthread_create(&wm_events_thread, NULL, start_wm_events_thread, state);
        pthread_join(wm_events_thread, &wm_events_result);

        if ((intptr_t)wm_events_result != 0) {
                goto free_and_error;
        }

        free(arg_options);
        natwm_state_destroy(state);
        destroy_logger(natwm_logger);

        return EXIT_SUCCESS;

free_and_error:
        LOG_CRITICAL(natwm_logger, "Encountered error. Closing...");

        free(arg_options);
        natwm_state_destroy(state);
        destroy_logger(natwm_logger);

        return EXIT_FAILURE;
}
