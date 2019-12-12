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
#include "parser.h"

static void hashmap_free_callback(void *data)
{
        struct config_value *value = (struct config_value *)data;

        config_value_destroy(value);
}

/**
 * Once the variable has been parsed from the configuration file we need to
 * create the config_list item that will be passed into the parser
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

                if (string_to_number(value, &number) != NO_ERROR) {
                        LOG_ERROR(natwm_logger,
                                  "Invalid number '%s' found",
                                  value);

                        return NULL;
                }

                return config_value_create_number(key, number);
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

        return config_value_create_string(key, value_stripped);
}

static struct config_value *
variable_from_config_value(char *key, struct config_value *variable)
{
        if (variable->type == NUMBER) {
                return config_value_create_number(key, variable->data.number);
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

        return config_value_create_string(key, string);
}

static struct config_value *
resolve_variable(struct parser *parser, const char *variable_key, char *new_key)
{
        struct map_entry *variable = map_get(parser->variables, variable_key);

        if (variable == NULL) {
                LOG_ERROR(natwm_logger,
                          "'%s' is not defined - Line: %zu",
                          variable_key,
                          parser->line_num);

                return NULL;
        }

        struct config_value *new
                = variable_from_config_value(new_key, variable->value);

        if (new == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to resolve variable '%s' - Line: %zu",
                          variable_key,
                          parser->line_num);

                return NULL;
        }

        return new;
}

/**
 * Handle the parsing of keys
 *
 * The parser buffer will be pointing to the beginning of a string something
 * like this:
 *
 * key = value
 *
 * We must pull out the key and then point the buffer to the EQUAL_CHAR
 */
static enum natwm_error handle_context_item_key(struct parser *parser,
                                                char **result, size_t *length)
{
        enum natwm_error err = GENERIC_ERROR;
        const char *line = parser->buffer + parser->pos;

        if (char_to_token(line[0]) != ALPHA_CHAR) {
                LOG_ERROR(natwm_logger,
                          "Invalid Key: '%c' - Line: %zu Col: %zu",
                          line[0],
                          parser->line_num,
                          parser->col_num);

                return INVALID_INPUT_ERROR;
        }

        // Find the EQUAL_CHAR which will allows us to know the bounds of the
        // key
        size_t equal_pos = 0;
        char *key = NULL;

        err = string_get_delimiter(line, '=', &key, &equal_pos, false);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Missing '=' - Line: %zu",
                          parser->line_num);

                return INVALID_INPUT_ERROR;
        }

        // We should now be able to strip the spaces around the key which will
        // leave us with a valid key
        char *stripped_key = NULL;
        size_t stripped_key_length = 0;

        err = string_strip_surrounding_spaces(
                key, &stripped_key, &stripped_key_length);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid config value - Line %zu",
                          parser->line_num);

                free(key);
                free(stripped_key);

                return INVALID_INPUT_ERROR;
        }

        // Cleanup the intermediate key
        free(key);

        // Update the buffer position to the equal pos
        parser_move(parser, equal_pos);

        // Now we can return our valid key and key length
        *result = stripped_key;
        *length = stripped_key_length;

        return NO_ERROR;
}

/**
 * Handle the parsing of a config item's value
 *
 * The parser buffer will be pointing to the beginning of a string something
 * like this:
 *
 * = <value>
 *
 * We will need to ignore the EQUAL_CHAR and stripped the spaces around the
 * value. We will then return the string back to the caller who can deal with
 * turning it into a config_value
 */
static enum natwm_error handle_context_item_value(struct parser *parser,
                                                  char **result, size_t *length)
{
        enum natwm_error err = GENERIC_ERROR;
        // We need to ignore the EQUAL_CHAR
        const char *line = (parser->buffer + parser->pos) + 1;
        char *value = NULL;
        size_t end_pos = 0;

        err = string_get_delimiter(line, '\n', &value, &end_pos, false);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to read item value - Line %zu Col: %zu",
                          parser->line_num,
                          parser->col_num);

                return INVALID_INPUT_ERROR;
        }

        // Now we need to strip the surrounding spaces to be left with the value
        // string

        char *value_stripped = NULL;
        size_t value_stripped_length = 0;

        err = string_strip_surrounding_spaces(
                value, &value_stripped, &value_stripped_length);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Found invalid item value - Line %zu Col: %zu",
                          parser->line_num,
                          parser->col_num);

                free(value);

                return INVALID_INPUT_ERROR;
        }

        // Free intermediate values
        free(value);

        // Update the parser position to the end of the line
        parser_move(parser, end_pos);

        *result = value_stripped;
        *length = value_stripped_length;

        return NO_ERROR;
}

/**
 * Here we will handle the parsing of a simple boolean value of the form:
 *
 * "true" or "false"
 *
 * No other "falsey" values will be parsed as boolean.
 */
static enum natwm_error parse_boolean_value_string(struct parser *parser,
                                                   char *key, char *value,
                                                   struct config_value **result)
{
        bool value_result = false;

        if (string_no_case_compare(value, "true")) {
                value_result = true;
        } else if (string_no_case_compare(value, "false")) {
                value_result = false;
        } else {
                LOG_ERROR(natwm_logger,
                          "Invalid boolean value '%s' found - Line %zu",
                          value,
                          parser->line_num);

                return INVALID_INPUT_ERROR;
        }

        // We found a valid boolean string. Store it in a config_value
        struct config_value *config_value = malloc(sizeof(struct config_value));

        if (config_value == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        config_value->key = key;
        config_value->type = BOOLEAN;
        config_value->data.boolean = value_result;

        *result = config_value;

        return NO_ERROR;
}

/**
 * Here we will handle parsing array values
 *
 * Arrays are different in that they support multi line value strings. The
 * string we are going to receive into this function will terminate at the
 * first '\n' so we need to create our own "array value string" from the
 * parser
 *
 * That "array value string" will resemble something like this:
 *
 * [<value>,<value>,<value>]
 *
 * or
 *
 * [
 *     <value>,
 *     <value>,
 *     <value>,
 * ]
 *
 * Both of which should be treated the same and return the same result.
 *
 * The rules of arrays are:
 *
 * Each value must have the same data type. An array is invalid if any of the
 * value types differ from their siblings.
 *
 * New lines are ignored, and since we don't allow newlines in strings they
 * are invalid except for splitting the array items on different lines
 *
 * A commas followed by a ARRAY_END char is ignored
 */
static enum natwm_error parse_array_value_string(struct parser *parser,
                                                 char *key, char *value,
                                                 struct config_value **result)
{
        UNUSED_FUNCTION_PARAM(key);
        UNUSED_FUNCTION_PARAM(result);

        // Ignore ARRAY_START character
        char *value_string = NULL;

        // Allow handling of multiline array value strings
        if (char_to_token(value[strlen(value) - 1]) != ARRAY_END) {
                size_t end_pos = 0;
                const char *line = parser->buffer + parser->pos;
                enum natwm_error err = string_get_delimiter(
                        line, ']', &value_string, &end_pos, true);

                if (err != NO_ERROR) {
                        LOG_ERROR(
                                natwm_logger,
                                "Could not find ']' in array value - Line %zu",
                                parser->line_num);

                        return INVALID_INPUT_ERROR;
                }

                // We found a valid multiline array value string. But now we
                // need to update the parser position to point to the
                // end of the array value string
                parser_move(parser, end_pos);
        } else {
                // Our array exists on a single line
                value_string = string_init(value);
        }

        char *array_items_string = NULL;
        size_t array_items_string_size = 0;

        if (string_splice(value_string,
                          1,
                          strlen(value_string) - 1,
                          &array_items_string,
                          &array_items_string_size)
            != NO_ERROR) {
                return GENERIC_ERROR;
        }

        free(value_string);

        LOG_INFO(natwm_logger, "Array: %s", array_items_string);

        free(array_items_string);

        return NO_ERROR;
}

static enum natwm_error parse_value_string(struct parser *parser, char *key,
                                           char *value,
                                           struct config_value **result)
{
        UNUSED_FUNCTION_PARAM(parser);
        UNUSED_FUNCTION_PARAM(key);
        UNUSED_FUNCTION_PARAM(result);

        struct config_value *config_value = NULL;

        switch (char_to_token(value[0])) {
        case ALPHA_CHAR: {
                enum natwm_error err = parse_boolean_value_string(
                        parser, key, value, &config_value);

                if (err != NO_ERROR) {
                        return err;
                }

                break;
        }
        case ARRAY_START: {
                enum natwm_error err = parse_array_value_string(
                        parser, key, value, &config_value);

                if (err != NO_ERROR) {
                        return err;
                }

                break;
        }
        case NUMERIC_CHAR:
                LOG_INFO(natwm_logger, "Found number");
                break;
        case QUOTE:
                LOG_INFO(natwm_logger, "Found string");
                break;
        case VARIABLE_START:
                LOG_INFO(natwm_logger, "Found variable");
                break;
        default:
                return INVALID_INPUT_ERROR;
        }

        return NO_ERROR;
}

/**
 * Handle creating a config_value from a string
 *
 * The parser will be pointing to the beginning of a line containing a
 * config value in the format:
 *
 * config_key = config_value
 *
 * If the value starts with VARIABLE_START then  a lookup is performed
 * in the existing variables. If something is found then the value is
 * replaced by what was found - otherwise -1 is returned
 *
 * These are used to create the returned config_value. If there is an
 * error while parsing then NULL is returned and no memory is left
 * allocated
 */
static struct config_value *handle_context_item(struct parser *parser)
{
        char *key = NULL;
        size_t key_length = 0;

        if (handle_context_item_key(parser, &key, &key_length) != NO_ERROR) {
                return NULL;
        }

        char *value = NULL;
        size_t value_length = 0;

        if (handle_context_item_value(parser, &value, &value_length)
            != NO_ERROR) {
                free(key);

                return NULL;
        }

        parse_value_string(parser, key, value, NULL);

        struct config_value *ret = NULL;

        if (char_to_token(value[0]) == VARIABLE_START) {
                ret = resolve_variable(parser, value + 1, key);
        } else {
                ret = config_value_from_string(key, value);
        }

        if (ret == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to save '%s' - Line %zu Col: %zu",
                          key,
                          parser->line_num,
                          parser->col_num);

                goto free_and_error;
        }

        // The value can be freed now since the config_value will have it's own
        // copy
        free(value);

        return ret;

free_and_error:
        free(key);
        free(value);

        return NULL;
}

/**
 * Handle the creation of a variable in the configuration
 *
 * When this function is called the parser will be pointing to the
 * start of a variable declaration in the form of
 *
 * $variable_name = <variable_value>
 * ^
 * |
 * *-(parser->pos)
 *
 * Once the variable has been saved to the parser, the parser positon
 * should also be updated to point to the '\n' of the current
 * line, so that the next line can be consumed
 */
static int parse_context_variable(struct parser *parser)
{
        // Skip VARIABLE_START
        parser_increment(parser);

        struct config_value *value = handle_context_item(parser);

        if (value == NULL) {
                return -1;
        }

        if (map_insert(parser->variables, value->key, value) != NO_ERROR) {
                config_value_destroy(value);

                return -1;
        }

        return 0;
}

/**
 * Handle the creation of a config item in the configuration
 *
 * When this function is called the parser position will be pointing to the
 * start of the configuration item in the form of
 *
 * config_item = <value>
 * ^
 * |
 * *-(parser->pos)
 *
 * Once the config item has been saved to the parser, the parser position will
 * be updated to point to the '\n' at the end of the current line, which will
 * allow for the next line to be consumed
 */
static int parse_context_config_item(struct parser *parser,
                                     struct map **config_map)
{
        struct config_value *item = handle_context_item(parser);

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

static struct map *read_context(struct parser *parser)
{
        struct map *map = map_init();

        if (map == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed initializing configuration map");

                return NULL;
        }

        // Setup hash map
        map_set_entry_free_function(map, hashmap_free_callback);

        char c = '\0';
        while ((c = parser->buffer[parser->pos]) != '\0') {
                switch (char_to_token(c)) {
                case COMMENT_START:
                        parser_consume_line(parser);
                        break;
                case VARIABLE_START:
                        if (parse_context_variable(parser) != 0) {
                                goto handle_error;
                        }
                        break;
                case ALPHA_CHAR:
                        if (parse_context_config_item(parser, &map) != 0) {
                                goto handle_error;
                        }
                default:
                        break;
                }

                parser_increment(parser);
        }

        return map;

handle_error:
        LOG_ERROR(natwm_logger, "Error reading configuration file!");

        map_destroy(map);

        return NULL;
}

/**
 * Initialize the configuration file using a string
 *
 * Takes a string buffer and uses it to return the key value pairs
 * of the config
 */
struct map *config_read_string(const char *string, size_t size)
{
        struct parser *parser = parser_create(string, size);

        if (parser == NULL) {
                return NULL;
        }

        struct map *map = read_context(parser);

        parser_destroy(parser);

        if (map == NULL) {
                return NULL;
        }

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

        size_t file_size = 0;

        if (get_file_size(file, &file_size) != NO_ERROR) {
                goto close_file_and_error;
        }

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

struct config_value *config_find(const struct map *config_map, const char *key)
{
        struct map_entry *entry = map_get(config_map, key);

        if (entry == NULL) {
                return NULL;
        }

        return (struct config_value *)entry->value;
}

enum natwm_error config_find_number(const struct map *config_map,
                                    const char *key, intmax_t *result)
{
        struct config_value *value = config_find(config_map, key);

        if (value == NULL) {
                return NOT_FOUND_ERROR;
        }

        if (value->type != NUMBER) {
                return INVALID_INPUT_ERROR;
        }

        *result = value->data.number;

        return NO_ERROR;
}

intmax_t config_find_number_fallback(const struct map *config_map,
                                     const char *key, intmax_t fallback)
{
        intmax_t value = 0;

        if (config_find_number(config_map, key, &value) != NO_ERROR) {
                return fallback;
        }

        return value;
}

const char *config_find_string(const struct map *config_map, const char *key)
{
        struct config_value *value = config_find(config_map, key);

        if (value == NULL || value->type != STRING) {
                return NULL;
        }

        return value->data.string;
}
const char *config_find_string_fallback(const struct map *config_map,
                                        const char *key, const char *fallback)
{
        const char *string = config_find_string(config_map, key);

        if (string == NULL) {
                return fallback;
        }

        return string;
}

void config_destroy(struct map *config_map)
{
        map_destroy(config_map);
}
