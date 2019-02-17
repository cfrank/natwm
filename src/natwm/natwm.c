// Required for handling signals
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

#include <common/logger.h>

static void handle_connection_error(int error)
{
        const char *message;

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

        LOG_CRITICAL_SHORT(natwm_logger, message);
}

static xcb_connection_t *make_connection(int *screen_num)
{
        xcb_connection_t *connection = xcb_connect(NULL, screen_num);

        int connection_error = xcb_connection_has_error(connection);

        if (connection_error > 0) {
                handle_connection_error(connection_error);

                // Still need to free connection
                xcb_disconnect(connection);

                return NULL;
        }

        return connection;
}

static void signal_handler(int signum)
{
        if (signum == SIGHUP) {
                LOG_INFO_SHORT(natwm_logger, "Restarting natwm...");
                // TODO: Reload WM

                return;
        }

        // TODO: Handle gracefully closing WM
        LOG_CRITICAL_SHORT(natwm_logger, "Terminating natwm...");

        return;
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

static int start_natwm(void)
{
        int screen_num;

        xcb_connection_t *connection = make_connection(&screen_num);

        if (connection == NULL) {
                return -1;
        }

        LOG_INFO_SHORT(natwm_logger, "Successfully connected to X server");

        xcb_disconnect(connection);

        return 0;
}

int main(void)
{
        // Initialize the logger
        initialize_logger();

        // Catch and handle signals
        if (install_signal_handlers() < 0) {
                LOG_ERROR_SHORT(
                        natwm_logger,
                        "Failed to handle signals - This may cause problems!");
        }

        // Start the window manager
        if (start_natwm() < 0) {
                return EXIT_FAILURE;
        }

        destroy_logger(natwm_logger);

        return EXIT_SUCCESS;
}
