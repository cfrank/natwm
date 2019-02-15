#include <common/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcb/xcb.h>

static void handle_connection_error(xcb_connection_t *connection, int error)
{
        printf("%s", "There was an error connecting to the x server");

        xcb_disconnect(connection);
}

static xcb_connection_t *make_connection(void)
{
        int screenp;

        xcb_connection_t *connection = xcb_connect(NULL, &screenp);

        int connection_error = xcb_connection_has_error(connection);

        if (connection_error > 0) {
                handle_connection_error(connection, connection_error);

                return NULL;
        }

        return connection;
}

int main(void)
{
        // Initialize the logger
        initialize_logger();

        xcb_connection_t *connection = make_connection();

        if (connection == NULL) {
                exit(EXIT_FAILURE);
        }

        return EXIT_SUCCESS;
}
