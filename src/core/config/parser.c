// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>

#include <common/map.h>

#include "parser.h"
#include "value.h"

static void hashmap_free_callback(void *data)
{
        struct config_value *value = (struct config_value *)data;

        config_value_destroy(value);
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
 * Increment the parser position one step. If the current position of the
 * parser is a new line make sure to update the column and line numbers to
 * represent the new location
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
