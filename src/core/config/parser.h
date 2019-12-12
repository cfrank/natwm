// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#pragma once

#include <stddef.h>

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
void parser_increment(struct parser *parser);
void parser_move(struct parser *parser, size_t new_pos);
void parser_consume_line(struct parser *parser);
void parser_destroy(struct parser *parser);
