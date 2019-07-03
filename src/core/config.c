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
#include <common/util.h>
#include "config.h"

#define DEFAULT_LIST_SIZE 10

struct parser_context {
        const char *buffer;
        size_t buffer_size;
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

static struct config_list *create_list(void)
{
        struct config_list *list = malloc(sizeof(struct config_list));

        if (list == NULL) {
                return NULL;
        }

        list->length = 0;
        list->size = DEFAULT_LIST_SIZE;
        list->values = malloc(sizeof(struct config_value) * DEFAULT_LIST_SIZE);

        if (list->values == NULL) {
                free(list);

                return NULL;
        }

        return list;
}

/**
 * Double the size of the list so that it can store more items
 *
 * If we are able to grow the list then the resized list is placed into the
 * pointer passed by the caller and 0 is returned
 *
 * Otherwise -1 is returned and the caller is required to free the list
 * to avoid a leak
 */
static int list_grow(struct config_list **list)
{
        size_t new_size = (sizeof(struct config_list) * ((*list)->size * 2));
        struct config_list *new_list = realloc(list, new_size);

        if (new_list == NULL) {
                // Realloc failed caller will need to release old list
                return -1;
        }

        *list = new_list;

        return 0;
}

static int list_insert(struct config_list *list, struct config_value *value)
{
        if (list->length == list->size) {
                // Need to grow list
                if (list_grow(&list) < 0) {
                        // Could not grow list
                        free(list);

                        return -1;
                }
        }

        // List should be able to hold new items
        list->values[list->length] = value;
        ++list->length;

        return 0;
}

static struct config_value *list_find(struct config_list *list, const char *key)
{
        for (size_t i = 0; i < list->length; ++i) {
                if (strcmp(key, list->values[i]->key) == 0) {
                        return list->values[i];
                }
        }

        return NULL;
}

static struct config_value *create_number(char *key, intmax_t number)
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

static struct config_value *create_string(char *key, char *string)
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
static struct parser_context *initialize_parser_context(const char *buffer,
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
        context->variables = create_list();

        if (context->variables == NULL) {
                free(context);

                return NULL;
        }

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
static void move_parser_context(struct parser_context *context, size_t new_pos)
{
        for (size_t i = 0; i < new_pos; ++i) {
                increment_parser_context(context);
        }
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
static struct config_value *create_value_from_strings(char *key, char *value)
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

                return create_number(key, number);
        }

        size_t value_len = strlen(value);

        // Make sure the value ends with a closing quote
        if (char_to_token(value[value_len - 1]) != QUOTE) {
                LOG_ERROR(natwm_logger, "Missing closing quote for $%s", key);
                return NULL;
        }

        char *string_value = NULL;

        string_splice(value, &string_value, 1, (ssize_t)(value_len - 1));

        if (string_value == NULL) {
                return NULL;
        }

        return create_string(key, string_value);
}

static struct config_value *value_from_variable(char *key,
                                                struct config_value *variable)
{
        if (variable->type == NUMBER) {
                return create_number(key, variable->data.number);
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

        return create_string(key, string);
}

static struct config_value *resolve_variable(struct parser_context *context,
                                             const char *variable_key,
                                             char *new_key)
{
        struct config_value *variable
                = list_find(context->variables, variable_key);

        if (variable == NULL) {
                LOG_ERROR(natwm_logger,
                          "'%s' is not defined - Line: %zu",
                          variable_key,
                          context->line_num);

                return NULL;
        }

        struct config_value *new = value_from_variable(new_key, variable);

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
        ssize_t equal_pos = string_get_delimiter(string, '=', &key, false);

        if (string_strip_surrounding_spaces(key, &key_stripped) < 0) {
                LOG_ERROR(natwm_logger,
                          "Invalid value key - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                // Since we didn't check for the success of get_delim
                // this could be NULL - free anyway
                free(key);

                return NULL;
        }

        move_parser_context(context, (size_t)equal_pos);

        // Skip EQUAL char
        const char *value_start = context->buffer + context->pos + 1;
        char *value = NULL;
        char *value_stripped = NULL;
        ssize_t end_pos
                = string_get_delimiter(value_start, '\n', &value, false);

        if (string_strip_surrounding_spaces(value, &value_stripped) < 0) {
                LOG_ERROR(natwm_logger,
                          "Invalid value - Line: %zu Col: %zu",
                          context->line_num,
                          context->col_num);

                free(value);

                return NULL;
        }

        struct config_value *ret = NULL;

        if (char_to_token(value_stripped[0]) == VARIABLE_START) {
                ret = resolve_variable(
                        context, value_stripped + 1, key_stripped);
        } else {
                ret = create_value_from_strings(key_stripped, value_stripped);
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
        move_parser_context(context, (size_t)end_pos);

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
        increment_parser_context(context);

        struct config_value *variable = handle_context_value(context);

        if (variable == NULL) {
                return -1;
        }

        list_insert(context->variables, variable);

        struct config_value *val = list_find(context->variables, variable->key);

        if (val != NULL) {
                printf("Found Key: '%s'\n", val->key);
        } else {
                printf("Val is NULL\n");
        }

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
                                     struct config_list **list)
{
        struct config_value *item = handle_context_value(context);

        if (item == NULL || *list == NULL) {
                return -1;
        }

        if (list_insert(*list, item) != 0) {
                return -1;
        }

        return 0;
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

static struct config_list *handle_file(struct parser_context *context)
{
        struct config_list *list = create_list();

        if (list == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed initializing configuration list");

                return NULL;
        }

        char c = '\0';
        while ((c = context->buffer[context->pos]) != '\0') {
                switch (char_to_token(c)) {
                case COMMENT_START:
                        consume_line(context);
                        break;
                case VARIABLE_START:
                        if (parse_context_variable(context) != 0) {
                                goto handle_error;
                        }
                        break;
                case ALPHA_CHAR:
                        if (parse_context_config_item(context, &list) != 0) {
                                goto handle_error;
                        }
                default:
                        break;
                }

                increment_parser_context(context);
        }

        printf("Read a total of %zu lines...\n", context->line_num);

        return list;

handle_error:
        LOG_ERROR(natwm_logger, "Error reading configuration file!");

        destroy_config_list(list);

        return NULL;
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

        struct parser_context *context
                = initialize_parser_context(file_buffer, file_size);

        if (context == NULL) {
                return NULL;
        }

        struct config_list *ret = handle_file(context);

        destroy_parser_context(context);
        free(file_buffer);

        return ret;
}

void destroy_config_list(struct config_list *list)
{
        for (size_t i = 0; i < list->length; ++i) {
                destroy_config_value(list->values[i]);
        }

        free(list->values);
        free(list);
}

void destroy_config_value(struct config_value *value)
{
        if (value->type == STRING) {
                // For strings we need to free the data as well
                free(value->data.string);
        }

        free(value->key);
        free(value);
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

        struct config_list *config = parse_file(config_file);

        if (config == NULL) {
                fclose(config_file);

                return -1;
        }

        destroy_config_list(config);
        fclose(config_file);

        return 0;
}
