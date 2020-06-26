// Copyright 2020 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stddef.h>

#include <common/error.h>

#include "value.h"

struct parser {
        const char *buffer;
        size_t buffer_size;
        size_t pos;
        size_t line_num;
        size_t col_num;
        struct map *variables;
};

enum parser_token {
        ALPHA_CHAR,
        ARRAY_START,
        ARRAY_END,
        COMMENT_START,
        EQUAL,
        NEW_LINE,
        NUMERIC_CHAR,
        QUOTE,
        UNKNOWN,
        VARIABLE_START,
};

enum parser_token char_to_token(char c);

struct parser *parser_create(const char *buffer, size_t buffer_size);
enum natwm_error parser_create_variable(struct parser *parser);
const struct config_value *parser_find_variable(const struct parser *parser, const char *key);
struct config_value *parser_parse_value(struct parser *parser, char *value);
enum natwm_error parser_read_key(struct parser *parser, char **result, size_t *length);
enum natwm_error parser_read_value(struct parser *parser, char **result, size_t *length);
enum natwm_error parser_read_item(struct parser *parser, char **key_result,
                                  struct config_value **value_result);
void parser_increment(struct parser *parser);
void parser_move(struct parser *parser, size_t new_pos);
void parser_consume_line(struct parser *parser);
void parser_destroy(struct parser *parser);
