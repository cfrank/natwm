// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <common/constants.h>
#include <common/list.h>
#include <common/logger.h>
#include <common/map.h>
#include <common/stack.h>
#include <common/string.h>

#include "parser.h"

static void hashmap_free_callback(void *data)
{
        struct config_value *value = (struct config_value *)data;

        config_value_destroy(value);
}

/**
 * This is a array context aware delimiter search. We need to find delimiters
 * as they relate to the outer array context.
 *
 * If we were searching for the first ',' char in an array items string like:
 *
 * [1, 2, 3],[4, 5, 6],[7, 8, 9]
 *
 * ---------^
 *
 * we would not want to find the first ',' in the first nested array, but the
 * first ',' splitting the arrays
 */
static enum natwm_error
array_context_get_delimiter(const char *string, char delimiter, char **result,
                            size_t *length, bool consume)
{
        enum natwm_error err = GENERIC_ERROR;
        struct stack *stack = stack_create();
        size_t index = 0;
        char ch = '\0';

        while ((ch = string[index]) != '\0') {
                enum parser_token token = char_to_token(ch);

                if (token == ARRAY_START) {
                        stack_push(stack, &index);
                }

                if (token == ARRAY_END) {
                        // We just need to delete this
                        struct stack_item *stack_item = stack_pop(stack);

                        if (stack_item != NULL) {
                                stack_item_destroy(stack_item);
                        }
                }

                if (ch != delimiter) {
                        ++index;

                        continue;
                }

                // We found the delimiter - check if the stack is empty.
                if (!stack_has_item(stack)) {
                        // If we want to include the delimiter in the final
                        // string increment the index
                        if (consume) {
                                ++index;
                        }

                        ++index;

                        break;
                }

                // We found the delimiter, but we are not in the correct
                // context, so just incremement the index and continue
                ++index;

                continue;
        }

        if (ch == '\0') {
                // We did not find anything
                stack_destroy(stack);

                return NOT_FOUND_ERROR;
        }

        char *resulting_string = NULL;
        size_t string_length = 0;

        err = string_splice(
                string, 0, index - 1, &resulting_string, &string_length);

        if (err != NO_ERROR) {
                stack_destroy(stack);

                return err;
        }

        *result = resulting_string;
        *length = string_length;

        stack_destroy(stack);

        return NO_ERROR;
}

/**
 * Given the start of an array string we need to find the closing ARRAY_END
 * for the first ARRAY_START char. This is made more complicated because of the
 * potential presence of nested arrays
 *
 * For a string that looks like this:
 *
 * [[1,2,3],[4,5,6],[7,8,9]]
 *
 * ---(Expected result)----^
 *
 *  We will return the array string and index position of the closing
 *  ARRAY_END char
 */
static enum natwm_error get_array_string(const char *string, char **result,
                                         size_t *index_pos)
{
        enum natwm_error err = GENERIC_ERROR;
        char *raw_array_string = NULL;
        size_t raw_string_length = 0;

        err = array_context_get_delimiter(
                string, ']', &raw_array_string, &raw_string_length, true);

        if (err != NO_ERROR) {
                return err;
        }

        char *array_string = NULL;
        size_t array_string_length = 0;

        err = string_strip_surrounding_spaces(
                raw_array_string, &array_string, &array_string_length);

        if (err != NO_ERROR) {
                free(raw_array_string);

                return err;
        }

        free(raw_array_string);

        *result = array_string;
        *index_pos = raw_string_length - 1;

        return NO_ERROR;
}

/**
 * This is an array context aware array split. It splits the inner values
 * of an array keeping in mind that there might be nested arrays. Instead of
 * splitting on each ',' (which would happen if we used string_split) we keep
 * track of our position on the array (and nested arrays) and only return the
 * top level items.
 *
 * Firstly we build a list of top level value items. This allows us to figure
 * out the number of value items dynamically. Then we do a single allocation
 * for the returned array and destroy the intermediate list.
 *
 * The provided char array is populated with the result, and the provided
 * length pointer is populated with the number of value items we found
 */
static enum natwm_error array_split_value_items_string(const char *string,
                                                       char ***result,
                                                       size_t *length)
{
        struct list *list = list_create();

        if (list == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        size_t current_index = 0;

        for (;;) {
                char *item = NULL;
                size_t end_pos = 0;
                enum natwm_error err = array_context_get_delimiter(
                        string + current_index, ',', &item, &end_pos, false);

                if (err == NOT_FOUND_ERROR) {
                        char *last_item = string_init(string + current_index);

                        if (last_item == NULL) {
                                goto free_and_error;
                        }

                        list_insert(list, last_item);

                        break;
                }

                if (err != NO_ERROR) {
                        goto free_and_error;
                }

                list_insert(list, item);

                string += (end_pos + 1);
        }

        char **array_items = malloc(sizeof(char *) * list->size);

        if (array_items == NULL) {
                goto free_and_error;
        }

        // Walk the list backwards to insert in the correct position
        size_t array_item_index = (list->size - 1);

        LIST_FOR_EACH(list, item)
        {
                char *list_item = (char *)item->data;

                array_items[array_item_index] = list_item;

                --array_item_index;
        }

        *result = array_items;
        *length = list->size;

        list_destroy(list);

        return NO_ERROR;

free_and_error:
        LIST_FOR_EACH(list, item)
        {
                if (item && item->data) {
                        free((char *)item->data);
                }
        }

        list_destroy(list);

        return GENERIC_ERROR;
}

/**
 * Take a value string which we know contains an array and return a string
 * containing all of the array value items.
 *
 * We allow arrays to span multiple lines so this function has to take that
 * into account and reparse the value string if we don't find an ARRAY_END
 * character at the end of the passed string
 *
 * The parser position is kept up to date in the case of a multiline value
 */
static enum natwm_error array_find_value_items_string(struct parser *parser,
                                                      char *string,
                                                      char **result,
                                                      size_t *length)
{
        // An array value string in the format
        // [<value>,<value>]
        char *array_value_string = NULL;

        if (char_to_token(string[strlen(string) - 1]) != ARRAY_END) {
                size_t end_pos = 0;
                const char *line = parser->buffer + parser->pos;

                enum natwm_error err
                        = get_array_string(line, &array_value_string, &end_pos);

                if (err != NO_ERROR) {
                        LOG_ERROR(natwm_logger,
                                  "Could not find ']' in array value string - "
                                  "Line %zu",
                                  parser->line_num);

                        return INVALID_INPUT_ERROR;
                }

                // We found a valid multiline array value string. But now we
                // need to update the parser position to point to the end of the
                // array value string
                parser_move(parser, end_pos);
        } else {
                // Our array exists on a single line
                array_value_string = string_init(string);
        }

        // From our value string we need to get the array items list
        // <value>,<value>
        char *array_value_items_string = NULL;
        size_t array_value_items_string_size = 0;
        enum natwm_error err = string_splice(array_value_string,
                                             1,
                                             strlen(array_value_string) - 1,
                                             &array_value_items_string,
                                             &array_value_items_string_size);

        if (err != NO_ERROR) {
                free(array_value_string);

                return err;
        }

        free(array_value_string);

        *result = array_value_items_string;
        *length = array_value_items_string_size;

        return NO_ERROR;
}

/**
 * Given an array of array value items we need to parse their values or resolve
 * the variables into values.
 *
 * We just take the value item and complete the same process as if we had found
 * a top level value string starting with "parser_parse_value"
 */
static struct config_value *
parser_resolve_array_values(struct parser *parser, char **value_items,
                            size_t value_items_length)
{
        struct config_value *config_value
                = config_value_create_array(value_items_length);

        if (config_value == NULL) {
                return NULL;
        }

        // Resolve and insert config_values into the array
        for (size_t i = 0; i < value_items_length; ++i) {
                struct config_value *item
                        = parser_parse_value(parser, value_items[i]);

                if (item == NULL) {
                        for (size_t j = i; j < value_items_length; ++j) {
                                free(value_items[j]);
                        }

                        config_value_destroy(config_value);

                        return NULL;
                }

                config_value->data.array->values[i] = item;
        }

        return config_value;
}

/**
 * Here we will handle parsing arrays
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
 */
static struct config_value *parser_parse_array(struct parser *parser,
                                               char *string)
{
        char *value_items_string = NULL;
        size_t value_items_string_length = 0;
        enum natwm_error err = GENERIC_ERROR;

        err = array_find_value_items_string(parser,
                                            string,
                                            &value_items_string,
                                            &value_items_string_length);

        if (err != NO_ERROR) {
                return NULL;
        }

        // We should now have a valid string containing the string
        // representation of the array values
        //
        // We now need to normalize it
        char **value_items = NULL;
        size_t value_items_length = 0;

        err = array_split_value_items_string(
                value_items_string, &value_items, &value_items_length);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Failed to parse array value items string - Line %zu",
                          parser->line_num);

                free(value_items_string);

                return NULL;
        }

        // Remove spaces around the items
        for (size_t i = 0; i < value_items_length; ++i) {
                char *value_item = NULL;
                err = string_strip_surrounding_spaces(
                        value_items[i], &value_item, NULL);

                // If we encounter an '' (empty string) as the last item in the
                // array we have two senarios:
                //
                // 1) The array is empty and there are no values
                // 2) There was a trailing comma
                //
                // In both cases we should free the string, decrement the array
                // length, and terminate the loop
                if (err == NOT_FOUND_ERROR && (i == value_items_length - 1)) {
                        free(value_items[i]);

                        value_items_length -= 1;

                        break;
                }

                if (err != NO_ERROR) {
                        LOG_ERROR(natwm_logger,
                                  "Failed to parse array value item '%s' - "
                                  "Line %zu",
                                  value_items[i],
                                  parser->line_num);

                        goto free_and_error;
                }

                free(value_items[i]);

                value_items[i] = value_item;
        }

        // Now we should have an array of stripped items
        // Last step is to parse them into a new config_value
        struct config_value *config_value = parser_resolve_array_values(
                parser, value_items, value_items_length);

        if (config_value == NULL) {
                free(value_items_string);
                free(value_items);

                return NULL;
        }

        free(string);
        free(value_items_string);
        free(value_items);

        return config_value;

free_and_error:
        // We need to free up our intermediate strings and the array of array
        // values we allocated
        free(value_items_string);
        free(value_items);

        return NULL;
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
static struct config_value *parser_resolve_variable(const struct parser *parser,
                                                    const char *variable_key)
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

        struct config_value *new = config_value_duplicate(variable);

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
 * The casing of the above values does not matter, just that they are
 * alphabetically equal
 *
 * No other "falsey" values will be parsed as boolean.
 */
static struct config_value *parser_parse_boolean(const struct parser *parser,
                                                 char *value)
{
        bool boolean = false;
        enum natwm_error err = string_to_boolean(value, &boolean);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid boolean value '%s' found - Line %zu",
                          value,
                          parser->line_num);

                return NULL;
        }

        struct config_value *config_value
                = config_value_create_boolean(boolean);

        if (config_value == NULL) {
                return NULL;
        }

        free(value);

        return config_value;
}

/**
 * Here we will handle the create of a simple numeric value
 */
static struct config_value *parser_parse_number(const struct parser *parser,
                                                char *value)
{
        intmax_t number = 0;
        enum natwm_error err = string_to_number(value, &number);

        if (err != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid numeric value '%s' found - Line %zu",
                          value,
                          parser->line_num);

                return NULL;
        }

        struct config_value *config_value = config_value_create_number(number);

        if (config_value == NULL) {
                return NULL;
        }

        // We no longer need this value
        free(value);

        return config_value;
}

/**
 * Here we will handle the parsing and creation of a variable value
 */
static struct config_value *parser_parse_variable(const struct parser *parser,
                                                  char *value)
{
        // we need to take the value (minus VARIABLE_START) and look it up
        // in the variable map. If it's found we need to duplicate it and store
        // it in a config item with the key passed in
        struct config_value *config_value
                = parser_resolve_variable(parser, value + 1);

        if (config_value == NULL) {
                return NULL;
        }

        // We no longer need this value since we only needed it for the
        // variable lookup
        free(value);

        return config_value;
}

/**
 * Here we will handle the creation of a simple string value
 */
static struct config_value *parser_parse_string(const struct parser *parser,
                                                char *string)
{
        // We first need to strip off the surrounding quotes from the string
        size_t string_len = strlen(string);
        char *stripped_string = NULL;

        if (string_splice(string, 1, string_len - 1, &stripped_string, NULL)
            != NO_ERROR) {
                LOG_ERROR(natwm_logger,
                          "Invalid string '%s' found - Line %zu",
                          string,
                          parser->line_num);

                return NULL;
        }

        struct config_value *config_value
                = config_value_create_string(stripped_string);

        if (config_value == NULL) {
                free(stripped_string);

                return NULL;
        }

        free(string);

        return config_value;
}

enum parser_token char_to_token(char c)
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

        map_set_setting_flag(parser->variables, MAP_FLAG_FREE_ENTRY_KEY);

        return parser;
}

/**
 * Handle the creation of a variable
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
enum natwm_error parser_create_variable(struct parser *parser)
{
        // Skip VARIABLE_START
        parser_increment(parser);

        enum natwm_error err = GENERIC_ERROR;
        char *key = NULL;
        struct config_value *value = NULL;

        err = parser_read_item(parser, &key, &value);

        if (err != NO_ERROR) {
                return err;
        }

        err = map_insert(parser->variables, key, value);

        if (err != NO_ERROR) {
                free(key);
                config_value_destroy(value);

                return err;
        }

        return NO_ERROR;
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
 * Here we will handle the parsing of a generic value.
 */
struct config_value *parser_parse_value(struct parser *parser, char *value)
{
        switch (char_to_token(value[0])) {
        case ALPHA_CHAR:
                return parser_parse_boolean(parser, value);
        case ARRAY_START:
                return parser_parse_array(parser, value);
        case NUMERIC_CHAR:
                return parser_parse_number(parser, value);
        case QUOTE:
                return parser_parse_string(parser, value);
        case VARIABLE_START:
                return parser_parse_variable(parser, value);
        default:
                return NULL;
        }
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

        SET_IF_NON_NULL(length, stripped_key_length);

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

        SET_IF_NON_NULL(length, value_stripped_length);

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
enum natwm_error parser_read_item(struct parser *parser, char **key_result,
                                  struct config_value **value_result)
{
        enum natwm_error err = GENERIC_ERROR;
        char *key = NULL;

        err = parser_read_key(parser, &key, NULL);

        if (err != NO_ERROR) {
                return err;
        }

        char *value = NULL;

        err = parser_read_value(parser, &value, NULL);

        if (err != NO_ERROR) {
                free(key);

                return err;
        }

        struct config_value *config_value = parser_parse_value(parser, value);

        if (config_value == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed to save '%s' - Line %zu",
                          key,
                          parser->line_num);

                free(key);
                free(value);

                return GENERIC_ERROR;
        }

        *key_result = key;
        *value_result = config_value;

        return NO_ERROR;
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
