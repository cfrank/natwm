// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/util.h>
#include "config.h"

struct parser_context {
        const char *buffer;
        size_t pos;
        size_t line_num;
        size_t col_num;
        struct config_list *variables;
};

enum config_token_types {
        ALPHA_CHAR,
        COMMENT_START,
        EQUAL,
        NEW_LINE,
        NUMERIC_CHAR,
        QUOTE,
        UNKNOWN,
        VARIABLE_START,
};

static enum config_token_types char_to_token(char c);

void destroy_config_list(struct config_list *list)
{
        for (size_t i = 0; i < list->length; ++i) {
                free(list->values[i]);
        }

        free(list);
}

/**
 * Initialize the parser context with the file buffer.
 *
 * This will be used to track the position of the parser in the file.
 */
static struct parser_context *initialize_parser_context(const char *buffer)
{
        struct parser_context *context = malloc(sizeof(struct parser_context));

        if (context == NULL) {
                return NULL;
        }

        context->buffer = buffer;
        context->pos = 0;
        context->line_num = 1;
        context->col_num = 1;
        context->variables = NULL;

        return context;
}

static void destroy_parser_context(struct parser_context *context)
{
        if (context->variables != NULL) {
                destroy_config_list(context->variables);
        }

        free(context);
}

/**
 * Increment the parser context one step. If the current position of the
 * context is a new line make sure to update the column and line numbers
 * to represent the new location
 */
static void increment_parser_context(struct parser_context *context)
{
        if (context->buffer[context->pos] == '\n') {
                ++context->line_num;
                context->col_num = 1;
        } else {
                ++context->col_num;
        }

        ++context->pos;
}

/**
 * Consume the rest of a line in the configuration file.
 */
static void consume_line(struct parser_context *context)
{
        while (context->buffer[context->pos] != '\n') {
                increment_parser_context(context);
        }
}

/**
 * Handle the creation of a variable in the configuration
 *
 * When this function is called the context will be pointing to the
 * start of a variable declaration in the form of
 *
 * $variable_name = <variable_value>
 * ^
 * |
 * *-(parser->pos)
 *
 * Once the variable has been saved to the context, the context
 * should also be updated to point to the '\n' of the current
 * line, so that the next line can be consumed
 */
static int handle_variable_creation(struct parser_context *context)
{
        // Skip to the variable name
        increment_parser_context(context);

        const char *variable_string = context->buffer + context->pos;

        if (char_to_token(variable_string[0]) != ALPHA_CHAR) {
                LOG_ERROR(natwm_logger,
                          "Invalid char found: '%c' - Line: %zu Col: %zu",
                          variable_string[0],
                          context->line_num,
                          context->col_num);
                return -1;
        }

        char *name = NULL;
        ssize_t name_length
                = string_get_delimiter(variable_string, '=', &name, false);

        if (name_length < 0) {
                LOG_ERROR(natwm_logger,
                          "Invalid variable declaration - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                return -1;
        }

        char *key = NULL;
        ssize_t key_length = string_strip_surrounding_spaces(name, &key);

        if (key_length < 0) {
                LOG_ERROR(natwm_logger,
                          "Invalid variable declaration - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);
        }

        printf("Found Key = '%s'\n", key);

        free(name);
        free(key);

        return 0;
}

/**
 * Take a char and return the corresponding token
 */
static enum config_token_types char_to_token(char c)
{
        switch (c) {
        case '/':
                return COMMENT_START;
        case '=':
                return EQUAL;
        case '\n':
                return NEW_LINE;
        case '"':
                return QUOTE;
        case '$':
                return VARIABLE_START;
        default:
                if (isdigit(c)) {
                        return NUMERIC_CHAR;
                }

                if (isalpha(c)) {
                        return ALPHA_CHAR;
                }

                return UNKNOWN;
        }
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
 * This can either be supplied by the caller (through the first
 * argument) or we can use the default location.
 *
 * If neither open then we return null
 */
static FILE *open_config_file(const char *path)
{
        FILE *file = NULL;

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
 * Takes an uninitalized string and places the contents of a file into
 * it
 *
 * The buffer is initialized with file_size + 1 bytes and must be freed
 * by the caller
 *
 * file_size bytes are read from file and placed in the buffer. The
 * buffer is then appended with a null terminator
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

static int handle_file(struct parser_context *context)
{
        char c = '\0';
        while ((c = context->buffer[context->pos]) != '\0') {
                switch (char_to_token(c)) {
                case COMMENT_START:
                        consume_line(context);
                        break;
                case VARIABLE_START:
                        if (handle_variable_creation(context) != 0) {
                                goto handle_error;
                        };
                        break;
                default:
                        break;
                }

                increment_parser_context(context);
        }

        printf("Read a total of %zu lines...\n", context->line_num);

        return 0;

handle_error:
        LOG_ERROR(natwm_logger, "Error reading configuration file!");

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

        char *file_buffer = NULL;

        if (read_file_into_buffer(file, &file_buffer, file_size) != 0) {
                return NULL;
        }

        struct parser_context *context = initialize_parser_context(file_buffer);

        if (context == NULL) {
                return NULL;
        }

        handle_file(context);

        destroy_parser_context(context);
        free(file_buffer);

        return NULL;
}

/**
 * Initialize the configuration file.
 *
 * This will return the file configuration pairs which can be used to
 * read the configuration
 */
int initialize_config(const char *path)
{
        FILE *config_file = open_config_file(path);

        if (config_file == NULL) {
                return -1;
        }

        if (parse_file(config_file) == NULL) {
                fclose(config_file);

                return -1;
        }

        fclose(config_file);

        return 0;
}