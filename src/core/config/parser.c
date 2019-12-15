// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/map.h>
#include <common/string.h>

#include "parser.h"

static void hashmap_free_callback(void *data)
{
        struct config_value *value = (struct config_value *)data;

        config_value_destroy(value);
}

/**
 * Take a value string which we know contains an array and return a string
 * containing all of the arrays items.
 *
 * We allow arrays to span multiple lines so this function has to take that
 * into account and reparse the value string if we don't find an ARRAY_END
 * character at the end of the passed string
 *
 * The parser position is kept up to date in the case of a multiline value
 */
static enum natwm_error get_array_items_string(struct parser *parser,
                                               char *string, char **result,
                                               size_t *length)
{
        // An array string in the format
        // [<value>,<value>]
        char *array_string = NULL;

        if (char_to_token(string[strlen(string) - 1]) != ARRAY_END) {
                size_t end_pos = 0;
                const char *line = parser->buffer + parser->pos;
                enum natwm_error err = string_get_delimiter(
                        line, ']', &array_string, &end_pos, true);

                if (err != NO_ERROR) {
                        LOG_ERROR(
                                natwm_logger,
                                "Could not find ']' in array value - Line %zu",
                                parser->line_num);

                        return INVALID_INPUT_ERROR;
                }

                // We found a valid multiline array value string. But now we
                // need to update the parser position to point to the end fo the
                // array value string
                parser_move(parser, end_pos);
        } else {
                // Our array exists on a single line
                array_string = string_init(string);
        }

        // From our array string we need to get a list of values
        // <value>,<value>
        char *values_string = NULL;
        size_t values_string_size = 0;
        enum natwm_error err = string_splice(array_string,
                                             1,
                                             strlen(array_string) - 1,
                                             &values_string,
                                             &values_string_size);

        if (err != NO_ERROR) {
                return err;
        }

        free(array_string);

        *result = values_string;
        *length = values_string_size;

        return NO_ERROR;
}

enum config_token char_to_token(char c)
{
        switch (c) {
        case '[':
                return ARRAY_START;
        case ']':
                return ARRAY_END;
        case '#':
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
 * Initialize the parser with the file buffer.
 *
 * This will be used to track the position of the parser in the file.
 */
struct parser *parser_create(const char *buffer, size_t buffer_size)
{
        struct parser *parser = malloc(sizeof(struct parser));

        if (parser == NULL) {
                return NULL;
        }

        parser->buffer = buffer;
        parser->buffer_size = buffer_size;
        parser->pos = 0;
        parser->line_num = 1;
        parser->col_num = 1;
        parser->variables = map_init();

        if (parser->variables == NULL) {
                free(parser);

                return NULL;
        }

        // Set up hashmap
        map_set_entry_free_function(parser->variables, hashmap_free_callback);

        return parser;
}

/**
 * Return a config value from the parser variable map
 */
const struct config_value *parser_find_variable(const struct parser *parser,
                                                const char *key)
{
        struct map_entry *entry = map_get(parser->variables, key);

        if (entry == NULL) {
                return NULL;
        }

        struct config_value *value = (struct config_value *)entry->value;

        return value;
}

/**
 * Resolve a variable found in the configuration file
 *
 * Once we have read and parsed a configuration item pointing to a variable
 * we need to resolve the variable.
 *
 * First we find the variable in the parser's variable map then we copy the
 * value.
 */
struct config_value *parser_resolve_variable(const struct parser *parser,
                                             const char *variable_key,
                                             char *new_key)
{
        const struct config_value *variable
                = parser_find_variable(parser, variable_key);

        if (variable == NULL) {
                LOG_ERROR(natwm_logger,
                          "'%s' is not defined - Line: %zu",
                          variable_key,
                          parser->line_num);

                return NULL;
        }

        struct config_value *new = config_value_duplicate(variable, new_key);

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
 * Here we will handle the creation of a simple boolean value of the form:
 *
 * "true" or "false"
 *
 * No other "falsey" values will be parsed as boolean.
 */
enum natwm_error parser_parse_boolean(struct parser *parser, char *key,
                                      char *value, struct config_value **result)
{
        bool boolean = false;
        enum natwm_error err = string_to_boolean(value, &boolean);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid boolean value '%s' found - Line %zu",
                          value,
                          parser->line_num);

                return err;
        }

        struct config_value *config_value
                = config_value_create_boolean(key, boolean);

        *result = config_value;

        return NO_ERROR;
}

enum natwm_error parser_parse_value(struct parser *parser, char *key,
                                    char *value, struct config_value **result)
{
        UNUSED_FUNCTION_PARAM(parser);
        UNUSED_FUNCTION_PARAM(key);
        UNUSED_FUNCTION_PARAM(result);

        enum natwm_error err = GENERIC_ERROR;
        struct config_value *config_value = NULL;

        switch (char_to_token(value[0])) {
        case ALPHA_CHAR:
                err = parser_parse_boolean(parser, key, value, &config_value);
                break;
        case ARRAY_START:
                err = parser_read_array(parser, key, value, &config_value);
                break;
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
 * Read a key from the current parser buffer
 *
 * The parser buffer will be pointing to the beginning of a string something
 * like this:
 *
 * key = value
 *
 * We must pull out the key and then point the buffer to the EQUAL_CHAR
 */
enum natwm_error parser_read_key(struct parser *parser, char **result,
                                 size_t *length)
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
 * Read a value from the parser buffer
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
enum natwm_error parser_read_value(struct parser *parser, char **result,
                                   size_t *length)
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
enum natwm_error parser_read_array(struct parser *parser, char *key,
                                   char *value, struct config_value **result)
{
        UNUSED_FUNCTION_PARAM(key);
        UNUSED_FUNCTION_PARAM(result);

        char *items_string = NULL;
        size_t items_string_length = 0;
        enum natwm_error err = GENERIC_ERROR;

        err = get_array_items_string(
                parser, value, &items_string, &items_string_length);

        if (err != NO_ERROR) {
                return err;
        }

        // We should now have a valid string containing the string
        // representation of the array values
        //
        // We now need to normalize it
        char **array_items = NULL;
        size_t array_items_length = 0;

        err = string_split(
                items_string, ',', &array_items, &array_items_length);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to parse array items - Line %zu",
                          parser->line_num);

                free(items_string);

                return err;
        }

        // Remove spaces around the items
        for (size_t i = 0; i < array_items_length; ++i) {
                char *stripped_item = NULL;
                err = string_strip_surrounding_spaces(
                        array_items[i], &stripped_item, NULL);

                if (err != NO_ERROR) {
                        goto free_and_error;
                }

                free(array_items[i]);

                array_items[i] = stripped_item;
        }

        // Now we should have an array of stripped items
        // Last step is to resolve them into a new config_value

free_and_error:
        // We need to free up our intermediate strings and the array of array
        // values we allocated
        free(items_string);

        for (size_t itr = 0; itr < array_items_length; ++itr) {
                if (array_items[itr]) {
                        free(array_items[itr]);
                }
        }

        free(array_items);

        return GENERIC_ERROR;
}

/**
 * Handle creating a config_value from a string
 *
 * The parser will be pointing to the beginning of a line containing a
 * config value in the format:
 *
 * config_key = config_value
 *
 * or in the case of array values (which allow multi-line values)
 *
 * config_key = [
 *      <value>,
 *      <value>,
 *]
 *
 * If the value starts with VARIABLE_START then  a lookup is performed
 * in the existing variables. If something is found then the value is
 * replaced by what was found - otherwise -1 is returned
 *
 * These are used to create the returned config_value. If there is an
 * error while parsing then NULL is returned and no memory is left
 * allocated
 */
struct config_value *parser_read_item(struct parser *parser)
{
        char *key = NULL;
        size_t key_length = 0;

        if (parser_read_key(parser, &key, &key_length) != NO_ERROR) {
                return NULL;
        }

        char *value = NULL;
        size_t value_length = 0;

        if (parser_read_value(parser, &value, &value_length) != NO_ERROR) {
                free(key);

                return NULL;
        }

        enum natwm_error err = GENERIC_ERROR;
        struct config_value *ret = NULL;

        LOG_INFO(natwm_logger, "Found item %s - %s", key, value);

        if (char_to_token(value[0]) == VARIABLE_START) {
                ret = parser_resolve_variable(parser, value + 1, key);
        } else {
                err = parser_parse_value(parser, key, value, &ret);
        }

        if (ret == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to save '%s' - Line %zu",
                          key,
                          parser->line_num);

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
 * Increment the parser position one step. If the current position of
 * the parser is a new line make sure to update the column and line
 * numbers to represent the new location
 */
void parser_increment(struct parser *parser)
{
        if (parser->pos > parser->buffer_size) {
                return;
        }

        if (parser->buffer[parser->pos] == '\n') {
                ++parser->line_num;
                parser->col_num = 1;
        } else {
                ++parser->col_num;
        }

        ++parser->pos;
}

/**
 * Move the parser position forward a specified number of bytes
 *
 * This makes sure that the parser positioning stays intact
 */
void parser_move(struct parser *parser, size_t new_pos)
{
        for (size_t i = 0; i < new_pos; ++i) {
                parser_increment(parser);
        }
}

/**
 * Consume the rest of a line in the configuration file.
 */
void parser_consume_line(struct parser *parser)
{
        char c = '\0';
        while ((c = parser->buffer[parser->pos]) != '\n' && c != '\0') {
                parser_increment(parser);
        }
}

void parser_destroy(struct parser *parser)
{
        if (parser->variables != NULL) {
                map_destroy(parser->variables);
        }

        free(parser);
}
