// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/util.h>
#include "config.h"

/**
 * Holds the parser context
 */
struct parser_context {
        const char *buffer;
        size_t pos;
        size_t line_num;
        size_t col_num;
        struct config_pair **variables;
};

/**
 * Initialize the parser context with the file buffer.
 *
 * This will be used to tract the position of the parser in the file.
 */
static struct parser_context *initialize_parser_context(const char *buffer)
{
        struct parser_context *context = malloc(sizeof(struct parser_context));

        if (context == NULL) {
                return NULL;
        }

        context->buffer = buffer;
        context->pos = 0;
        context->line_num = 0;
        context->col_num = 0;
        context->variables = NULL;

        return context;
}

static void destroy_parser_context(struct parser_context *context)
{
        if (context->variables != NULL) {
                // TODO: Use correct destroy function
                free(context->variables);
        }

        free(context);
}

/**
 * Handle creating number pairs
 */
static struct config_value *create_number(const char *key, int64_t number)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->key = key;
        value->type = NUMBER;
        value->data.number = number;

        return value;
}

/**
 * Handle creating string pairs
 */
static struct config_value *create_string(const char *key, const char *string)
{
        struct config_value *value = malloc(sizeof(struct config_value));

        if (value == NULL) {
                return NULL;
        }

        value->key = key;
        value->type = STRING;
        value->data.string = string;

        return value;
}

/**
 * Open a configuration file
 *
 * This can either be supplied by the caller (through the first argument)
 * or we can use the default location.
 *
 * If neither open then we return null
 */
static FILE *open_config_file(const char *path)
{
        FILE *file;

        // If the user supplies a path use that one
        if (path != NULL) {
                file = fopen(path, "r");

                if (file == NULL) {
                        LOG_ERROR(natwm_logger,
                                  "Failed to find configuration file at %s",
                                  path);

                        return NULL;
                }

                return file;
        }

        // Otherwise use the default location
        struct passwd *db = getpwuid(getuid());

        if (db == NULL) {
                LOG_ERROR(natwm_logger, "Failed to find HOME directory");

                return NULL;
        }

        char *config_path = alloc_string(db->pw_dir);

        string_append(&config_path, "/.config/");
        string_append(&config_path, NATWM_CONFIG_FILE);

        // Check if the file exists
        if (!path_exists(config_path)) {
                LOG_ERROR(natwm_logger,
                          "Failed to find configuration file at %s",
                          config_path);

                goto release_config_path_and_error;
        }

        file = fopen(config_path, "r");

        if (file == NULL) {
                LOG_ERROR(natwm_logger, "Failed to open %s", config_path);

                goto release_config_path_and_error;
        }

        free(config_path);

        return file;

release_config_path_and_error:
        free(config_path);

        return NULL;
}

/**
 * Takes an uninitalized string and places the contents of a file into it
 *
 * The buffer is initialized with file_size + 1 bytes and must be freed by the
 * caller
 *
 * file_size bytes are read from file and placed in the buffer. The buffer is
 * then appended with a null terminator
 */
static int read_file_into_buffer(FILE *file, char **buffer, size_t file_size)
{
        *buffer = malloc(file_size + 1);

        if (*buffer == NULL) {
                return -1;
        }

        size_t bytes_read = fread(*buffer, 1, file_size, file);

        if (bytes_read != file_size) {
                // Read the wrong amount of bytes
                free(*buffer);

                return -1;
        }

        (*buffer)[file_size] = '\0';

        return 0;
}

/**
 * Take the parsing context and use the current position to grab a line in the
 * configuration.
 *
 * Read the line and store the content
 *
 * Reset the pos on the context to the correct position of the next line
 */
static int read_line(struct parser_context *context)
{
        char c;

        while ((c = context->buffer[context->pos]) != '\n' && c != '\0') {
                printf("%c - %zu\n", c, context->col_num);
                context->col_num++;
                context->pos++;
        }

        // If we hit the end of the buffer just exit out
        if (c == '\0') {
                return -1;
        }

        // When we find a new line move to the next character
        // and update the current column/line
        if (c == '\n') {
                context->pos++;
                context->col_num = 0;
                context->line_num++;

                return 0;
        }

        return -1;
}

/**
 * Parse a configuration file and return the list of configuration pairs
 */
static struct config_list *parse_file(FILE *file)
{
        ssize_t ftell_result = get_file_size(file);

        if (ftell_result < 0) {
                return NULL;
        }

        size_t file_size = (size_t)ftell_result;

        char *file_buffer;

        if (read_file_into_buffer(file, &file_buffer, file_size) != 0) {
                return NULL;
        }

        struct parser_context *context = initialize_parser_context(file_buffer);

        if (context == NULL) {
                return NULL;
        }

        while (read_line(context) == 0) {
                printf("%s - %zu\n", "Read another line", context->line_num);
        }

        destroy_parser_context(context);
        free(file_buffer);

        return NULL;
}

/**
 * Initialize the configuration file.
 *
 * This will return the file configuration pairs which can be used to read the
 * configuration
 */
void initialize_config(const char *path)
{
        FILE *config_file = open_config_file(path);

        if (config_file == NULL) {
                return;
        }

        parse_file(config_file);

        fclose(config_file);
}
