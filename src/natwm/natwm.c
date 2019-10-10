#ifdef USE_POSIX
// Required for handling signals
#define _POSIX_C_SOURCE 200809L
#endif

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>

#include <common/logger.h>
#include <core/config.h>
#include <core/ewmh.h>
#include <core/map.h>
#include <core/state.h>

struct argument_options {
        const char *config_path;
        const char *screen;
        bool verbose;
};

enum status {
        STOPPED = 1 << 0,
        RUNNING = 1 << 1,
#ifdef USE_POSIX
        RELOAD = 1 << 2,
        NEEDS_RELOAD = RUNNING | RELOAD,
#endif
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

static void signal_handler(int signum)
{
#ifdef USE_POSIX
        if (signum == SIGHUP) {
                // Perform a reload
                program_status |= NEEDS_RELOAD;

                return;
        }
#endif
        program_status = STOPPED;
}

static int install_signal_handlers(void)
{
#ifdef USE_POSIX
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
#else
        // Install interupt handler using ansi signals

        if (signal(SIGINT, signal_handler) == SIG_ERR) {
                LOG_ERROR(natwm_logger, "Failed to handle SIGINT");

                return -1;
        }

        if (signal(SIGTERM, signal_handler) == SIG_ERR) {
                LOG_ERROR(natwm_logger, "Failed to handle SIGTERM");

                return -1;
        }

        return 0;
#endif
}

static int start_natwm(struct natwm_state *state, const char *config_path)
{
        while (program_status & RUNNING) {
                struct config_value *val = config_find(state->config, "author");

                LOG_INFO(natwm_logger, val->data.string);

                sleep(1);
#ifdef USE_POSIX
                if (program_status & RELOAD) {
                        LOG_INFO(natwm_logger, "Reloading natwm...");

                        // Destroy the old config
                        destroy_config(state->config);

                        // Re-initialize the new config
                        struct map *new_config
                                = initialize_config_path(config_path);

                        if (new_config == NULL) {
                                LOG_ERROR(natwm_logger,
                                          "Failed to reload configuarion!");

                                return -1;
                        }

                        state->config = new_config;

                        program_status &= (unsigned int)~RELOAD;
                }
#endif
        }

        // Event loop stopped disconnect from x
        LOG_INFO(natwm_logger, "Disconnected...");

        return 0;
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
        struct natwm_state *state = natwm_state_init();

        if (state == NULL) {
                LOG_CRITICAL(natwm_logger,
                             "Failed to initialize applicaiton state");

                exit(EXIT_FAILURE);
        }

        // Initialize config
        struct map *config = initialize_config_path(arg_options->config_path);

        if (config == NULL) {
                goto free_and_error;
        }

        state->config = config;

        // Catch and handle signals
        if (install_signal_handlers() < 0) {
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

        xcb_ewmh_connection_t *ewmh = ewmh_init(xcb);

        if (ewmh == NULL) {
                goto free_and_error;
        }

        state->ewmh = ewmh;

        program_status = RUNNING;

        if (start_natwm(state, arg_options->config_path) < 0) {
                goto free_and_error;
        }

        free(arg_options);
        destroy_logger(natwm_logger);
        natwm_state_destroy(state);

        return EXIT_SUCCESS;

free_and_error:
        free(arg_options);
        destroy_logger(natwm_logger);
        natwm_state_destroy(state);

        return EXIT_FAILURE;
}
