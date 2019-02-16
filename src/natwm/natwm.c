#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

#include <common/logger.h>

static void handle_connection_error(xcb_connection_t *connection, int error)
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

        xcb_disconnect(connection);
}

static xcb_connection_t *make_connection(int *screen_num)
{
        xcb_connection_t *connection = xcb_connect(NULL, screen_num);

        int connection_error = xcb_connection_has_error(connection);

        if (connection_error > 0) {
                handle_connection_error(connection, connection_error);

                return NULL;
        }

        return connection;
}

int main(void)
{
        int screen_num;

        // Initialize the logger
        initialize_logger();

        xcb_connection_t *connection = make_connection(&screen_num);

        if (connection == NULL) {
                exit(EXIT_FAILURE);
        }

        LOG_INFO_SHORT(natwm_logger, "Successfully connected to X server");

        xcb_disconnect(connection);

        return EXIT_SUCCESS;
}
