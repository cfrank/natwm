// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/string.h>
#include <common/util.h>
#include "config.h"

struct parser_context {
        const char *buffer;
        size_t buffer_size;
        size_t pos;
        size_t line_num;
        size_t col_num;
        struct map *variables;
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
 * This is purely for use by the map holding config values
 *
 * a config_value_destroy is exposed below
 */
static void internal_config_value_destroy(void *data)
{
        struct config_value *value = (struct config_value *)data;

        if (value->type == STRING) {
                // For strings we need to free the data as well
                free(value->data.string);
        }

        free(value->key);
        free(value);
}

static struct config_value *create_number_value(char *key, intmax_t number)
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

static struct config_value *create_string_value(char *key, char *string)
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
 * Initialize the parser context with the file buffer.
 *
 * This will be used to track the position of the parser in the file.
 */
static struct parser_context *parser_context_init(const char *buffer,
                                                  size_t buffer_size)
{
        struct parser_context *context = malloc(sizeof(struct parser_context));

        if (context == NULL) {
                return NULL;
        }

        context->buffer = buffer;
        context->buffer_size = buffer_size;
        context->pos = 0;
        context->line_num = 1;
        context->col_num = 1;
        context->variables = map_init();

        if (context->variables == NULL) {
                free(context);

                return NULL;
        }

        // Set up hashmap
        map_set_entry_free_function(context->variables,
                                    internal_config_value_destroy);

        return context;
}

static void parser_context_destroy(struct parser_context *context)
{
        if (context->variables != NULL) {
                map_destroy(context->variables);
        }

        free(context);
}

/**
 * Increment the parser context one step. If the current position of the
 * context is a new line make sure to update the column and line numbers
 * to represent the new location
 */
static void parser_context_increment(struct parser_context *context)
{
        if (context->pos > context->buffer_size) {
                return;
        }

        if (context->buffer[context->pos] == '\n') {
                ++context->line_num;
                context->col_num = 1;
        } else {
                ++context->col_num;
        }

        ++context->pos;
}

/**
 * Move the parser context forward a specified number of bytes
 *
 * This makes sure that the parser positioning stays intact
 */
static void parser_context_move(struct parser_context *context, size_t new_pos)
{
        for (size_t i = 0; i < new_pos; ++i) {
                parser_context_increment(context);
        }
}

/**
 * Consume the rest of a line in the configuration file.
 */
static void parser_context_consume_line(struct parser_context *context)
{
        char c = '\0';
        while ((c = context->buffer[context->pos]) != '\n' && c != '\0') {
                parser_context_increment(context);
        }
}

/**
 * Once the variable has been parsed from the configuration file we need to
 * create the config_list item that will be passed into the context
 *
 * The arguments supplied to this function are:
 *
 * Key -> The key string which should valid
 *
 * Value -> The value which needs to be parsed further to determine it's type
 * and strip any additional characters
 */
static struct config_value *config_value_from_string(char *key, char *value)
{
        // Handle creating numbers
        if (char_to_token(value[0]) != QUOTE) {
                // TODO: Handle double values
                intmax_t number = 0;

                if (string_to_number(value, &number) != 0) {
                        LOG_ERROR(natwm_logger,
                                  "Invalid number '%s' found",
                                  value);

                        return NULL;
                }

                return create_number_value(key, number);
        }

        size_t value_len = strlen(value);

        // Make sure the value ends with a closing quote
        if (char_to_token(value[value_len - 1]) != QUOTE) {
                LOG_ERROR(natwm_logger, "Missing closing quote for $%s", key);
                return NULL;
        }

        char *value_stripped = NULL;
        size_t size = 0;

        enum natwm_error err = string_splice(
                value, 1, (value_len - 1), &value_stripped, &size);

        if (err != NO_ERROR) {
                return NULL;
        }

        return create_string_value(key, value_stripped);
}

static struct config_value *
variable_from_config_value(char *key, struct config_value *variable)
{
        if (variable->type == NUMBER) {
                return create_number_value(key, variable->data.number);
        }

        // Copy variable
        if (variable->data.string == NULL) {
                return NULL;
        }

        size_t length = strlen(variable->data.string);

        char *string = malloc(length + 1);

        if (string == NULL) {
                return NULL;
        }

        memcpy(string, variable->data.string, length + 1);

        return create_string_value(key, string);
}

static struct config_value *resolve_variable(struct parser_context *context,
                                             const char *variable_key,
                                             char *new_key)
{
        struct map_entry *variable = map_get(context->variables, variable_key);

        if (variable == NULL) {
                LOG_ERROR(natwm_logger,
                          "'%s' is not defined - Line: %zu",
                          variable_key,
                          context->line_num);

                return NULL;
        }

        struct config_value *new
                = variable_from_config_value(new_key, variable->value);

        if (new == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to resolve variable '%s' - Line: %zu",
                          variable_key,
                          context->line_num);

                return NULL;
        }

        return new;
}

/**
 * Handle creating a config_value from a string
 *
 * The context will be pointing to the beginning of a line containing a config
 * value in the format:
 *
 * config_key = config_value
 *
 * It is required that the first character pointed to by the context is a
 * ALPHA_CHAR
 *
 * After that the key is determined by parsing until a EQUAL char is found. The
 * key is then stripped of surrounding spaces and the result is the final key.
 *
 * The value is then determincontext_variableNEW_LINE char is found. The
 * value is then also stripped of surrounding spaces.
 *
 * If the value starts with VARIABLE_START then  a lookup is performed in the
 * existing variables. If something is found then the value is replaced by what
 * was found - otherwise -1 is returned
 *
 * These are used to create the returned config_value. If there is an error
 * while parsing then NULL is returned and no memory is left allocated
 */
static struct config_value *handle_context_value(struct parser_context *context)
{
        const char *string = context->buffer + context->pos;
        enum natwm_error err = GENERIC_ERROR;

        if (char_to_token(string[0]) != ALPHA_CHAR) {
                LOG_ERROR(natwm_logger,
                          "Invalid Value: '%c' - Line: %zu Col: %zu",
                          string[0],
                          context->line_num,
                          context->col_num);

                return NULL;
        }

        char *key = NULL;
        char *key_stripped = NULL;
        size_t equal_pos = 0;
        err = string_get_delimiter(string, '=', &key, &equal_pos, false);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid variable - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                return NULL;
        }

        err = string_strip_surrounding_spaces(key, &key_stripped, NULL);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid value key - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                free(key);

                return NULL;
        }

        parser_context_move(context, equal_pos);

        // Skip EQUAL char
        const char *value_start = context->buffer + context->pos + 1;
        char *value = NULL;
        char *value_stripped = NULL;
        size_t end_pos = 0;

        err = string_get_delimiter(value_start, '\n', &value, &end_pos, false);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid value - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                return NULL;
        }

        err = string_strip_surrounding_spaces(value, &value_stripped, NULL);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid value - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                goto free_and_error;
        }

        struct config_value *ret = NULL;

        if (char_to_token(value_stripped[0]) == VARIABLE_START) {
                ret = resolve_variable(
                        context, value_stripped + 1, key_stripped);
        } else {
                ret = config_value_from_string(key_stripped, value_stripped);
        }

        if (ret == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to save '%s' - Line %zu Col: %zu",
                          key_stripped,
                          context->line_num,
                          context->col_num);

                goto free_and_error;
        }

        // Update context
        parser_context_move(context, end_pos);

        // Free everything not contained in the value
        free(key);
        free(value);
        free(value_stripped);

        return ret;

free_and_error:
        free(key);
        free(key_stripped);
        free(value);
        free(value_stripped);

        return NULL;
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
static int parse_context_variable(struct parser_context *context)
{
        // Skip VARIABLE_START
        parser_context_increment(context);

        struct config_value *variable = handle_context_value(context);

        if (variable == NULL) {
                return -1;
        }

        map_insert(context->variables, variable->key, variable);

        return 0;
}

/**
 * Handle the creation of a config item in the configuration
 *
 * When this function is called the context will be pointing to the start
 * of the configuration item in the form of
 *
 * config_item = <value>
 * ^
 * |
 * *-(parser->pos)
 *
 * Once the config item has been saved to the context, the context will be
 * updated to point to the '\n' at the end of the current line, which will
 * allow for the next line to be consumed
 */
static int parse_context_config_item(struct parser_context *context,
                                     struct map **config_map)
{
        struct config_value *item = handle_context_value(context);

        if (item == NULL || *config_map == NULL) {
                return -1;
        }

        if (map_insert(*config_map, item->key, item) != NO_ERROR) {
                return -1;
        }

        return 0;
}

/**
 * Try to find the configuration file
 *
 * Uses a couple common locations and falls back to searching the .confg
 * directory in the users HOME directory
 */
static char *get_config_path(void)
{
        const char *base_directory = NULL;
        char *config_path = NULL;

        if ((base_directory = getenv("XDG_CONFIG_HOME")) != NULL) {
                config_path = string_init(base_directory);

                return config_path;
        }

        if ((base_directory = getenv("HOME")) != NULL) {
                config_path = string_init(base_directory);
                string_append(&config_path, "/.config/");

                return config_path;
        }

        // If we still haven't found anything use passwd
        struct passwd *db = getpwuid(getuid());

        if (db == NULL) {
                return NULL;
        }

        config_path = string_init(db->pw_dir);
        string_append(&config_path, "/.config/");

        return config_path;
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

        char *config_path = get_config_path();

        if (config_path == NULL) {
                LOG_ERROR(natwm_logger, "Failed to find HOME directory");

                return NULL;
        }

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

static struct map *read_context(struct parser_context *context)
{
        struct map *map = map_init();

        if (map == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed initializing configuration map");

                return NULL;
        }

        // Setup hash map
        map_set_entry_free_function(map, internal_config_value_destroy);

        char c = '\0';
        while ((c = context->buffer[context->pos]) != '\0') {
                switch (char_to_token(c)) {
                case COMMENT_START:
                        parser_context_consume_line(context);
                        break;
                case VARIABLE_START:
                        if (parse_context_variable(context) != 0) {
                                goto handle_error;
                        }
                        break;
                case ALPHA_CHAR:
                        if (parse_context_config_item(context, &map) != 0) {
                                goto handle_error;
                        }
                default:
                        break;
                }

                parser_context_increment(context);
        }

        return map;

handle_error:
        LOG_ERROR(natwm_logger, "Error reading configuration file!");

        map_destroy(map);

        return NULL;
}

struct config_value *config_find(const struct map *config_map, const char *key)
{
        struct map_entry *entry = map_get(config_map, key);

        if (entry == NULL) {
                return NULL;
        }

        return (struct config_value *)entry->value;
}

void config_destroy(struct map *config_map)
{
        map_destroy(config_map);
}

void config_value_destroy(struct config_value *value)
{
        internal_config_value_destroy(value);
}

/**
 * Initialize the configuration file using a string
 *
 * Takes a string buffer and uses it to return the key value pairs
 * of the config
 */
struct map *config_read_string(const char *string, size_t config_size)
{
        struct parser_context *context
                = parser_context_init(string, config_size);

        if (context == NULL) {
                return NULL;
        }

        struct map *map = read_context(context);

        parser_context_destroy(context);

        // May be NULL
        return map;
}

/**
 * Initialize the configuration file.
 *
 * Takes a path to a configuration file, opens it, reads the contents
 * into a buffer and uses it to return the key value pairs of the config
 * file
 */
struct map *config_initialize_path(const char *path)
{
        FILE *file = open_config_file(path);

        if (file == NULL) {
                return NULL;
        }

        ssize_t ftell_result = get_file_size(file);

        if (ftell_result < 0) {
                goto close_file_and_error;
        }

        size_t file_size = (size_t)ftell_result;
        char *file_buffer = NULL;

        if (read_file_into_buffer(file, &file_buffer, file_size) != 0) {
                goto close_file_and_error;
        }

        struct map *config = config_read_string(file_buffer, file_size);

        if (config == NULL) {
                free(file_buffer);

                goto close_file_and_error;
        }

        free(file_buffer);
        fclose(file);

        return config;

close_file_and_error:
        fclose(file);

        return NULL;
}
