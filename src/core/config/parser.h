// Copyright 2019 Chris Frank
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

enum config_token {
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

enum config_token char_to_token(char c);

struct parser *parser_create(const char *buffer, size_t buffer_size);
const struct config_value *parser_find_variable(const struct parser *parser,
                                                const char *key);
struct config_value *parser_resolve_variable(const struct parser *parser,
                                             const char *variable_key,
                                             char *new_key);
enum natwm_error parser_read_key(struct parser *parser, char **result,
                                 size_t *length);
enum natwm_error parser_read_value(struct parser *parser, char **result,
                                   size_t *length);
enum natwm_error parser_read_array(struct parser *parser, char *key,
                                   char *value, struct config_value **result);
struct config_value *parser_read_item(struct parser *parser);
void parser_increment(struct parser *parser);
void parser_move(struct parser *parser, size_t new_pos);
void parser_consume_line(struct parser *parser);
void parser_destroy(struct parser *parser);
