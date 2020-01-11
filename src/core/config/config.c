// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <pwd.h>
#include <stdlib.h>
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
static enum natwm_error config_item_create(struct parser *parser,
                                           struct map *config_map)
{
        enum natwm_error err = GENERIC_ERROR;
        char *key = NULL;
        struct config_value *item = NULL;

        err = parser_read_item(parser, &key, &item);

        if (err != NO_ERROR) {
                return err;
        }

        err = map_insert(config_map, key, item);

        if (err != NO_ERROR) {
                free(key);

                config_value_destroy(item);

                return err;
        }

        return NO_ERROR;
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

static struct map *config_parse(struct parser *parser)
{
        struct map *map = map_init();

        if (map == NULL) {
                LOG_ERROR(natwm_logger,
                          "Failed initializing configuration map");

                return NULL;
        }

        // Setup hash map
        map_set_entry_free_function(map, hashmap_free_callback);
        map_set_setting_flag(map, MAP_FLAG_FREE_ENTRY_KEY);

        char c = '\0';
        while ((c = parser->buffer[parser->pos]) != '\0') {
                switch (char_to_token(c)) {
                case COMMENT_START:
                        parser_consume_line(parser);

                        break;
                case VARIABLE_START:
                        if (parser_create_variable(parser) != NO_ERROR) {
                                goto handle_error;
                        }

                        break;
                case ALPHA_CHAR:
                        if (config_item_create(parser, map) != NO_ERROR) {
                                goto handle_error;
                        }

                        break;
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

        struct map *map = config_parse(parser);

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

enum natwm_error config_find_array(const struct map *config_map,
                                   const char *key,
                                   const struct config_array **result)
{
        struct config_value *value = config_find(config_map, key);

        if (value == NULL) {
                return NOT_FOUND_ERROR;
        }

        if (value->type != ARRAY) {
                return INVALID_INPUT_ERROR;
        }

        *result = value->data.array;

        return NO_ERROR;
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

enum natwm_error config_find_string(const struct map *config_map,
                                    const char *key, const char **result)
{
        struct config_value *value = config_find(config_map, key);

        if (value == NULL) {
                return NOT_FOUND_ERROR;
        }

        if (value->type != STRING) {
                return INVALID_INPUT_ERROR;
        }

        *result = value->data.string;

        return NO_ERROR;
}

const char *config_find_string_fallback(const struct map *config_map,
                                        const char *key, const char *fallback)
{
        const char *string = NULL;

        if (config_find_string(config_map, key, &string) != NO_ERROR) {
                return fallback;
        }

        return string;
}

void config_destroy(struct map *config_map)
{
        map_destroy(config_map);
}
