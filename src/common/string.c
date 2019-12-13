// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "list.h"
#include "logger.h"
#include "string.h"

// TODO: Move to common macro file if used again
#define SET_IF_NON_NULL(dest, value)                                           \
        if ((dest) != NULL) {                                                  \
                (*dest) = value;                                               \
        }

/*
 * Duplicates a stack allocated string to a heap allocated string. This allows
 * for mutating/building upon a string with for instance the string_append
 * function
 */
ATTR_NONNULL char *string_init(const char *string)
{
        // Include null terminator
        size_t length = strlen(string) + 1;
        char *result = malloc(length);

        if (result == NULL) {
                return NULL;
        }

        memcpy(result, string, length);

        return result;
}

/*
 * Takes a pointer to a heap allocated string and appends another string to the
 * end of it, making sure to add the null terminator to the end. The resulting
 * string is reallocated in place of the first argument, so it must be freed
 */
ATTR_NONNULL enum natwm_error string_append(char **destination,
                                            const char *append)
{
        size_t destination_size = strlen(*destination);
        size_t append_size = strlen(append);
        size_t result_size = destination_size + append_size;
        char *tmp = realloc(*destination, result_size + 1);

        if (tmp == NULL) {
                free(*destination);

                return MEMORY_ALLOCATION_ERROR;
        }

        *destination = tmp;

        strncpy(*destination + destination_size, append, append_size);

        (*destination)[result_size] = '\0';

        return NO_ERROR;
}

/*
 * Takes a pointer to a heap allocated string and appends a single char to the
 * end of it, making sure to add a null terminator to the end. The resulting
 * string is reallocated in place of the first argument, so it must be freed
 */
ATTR_NONNULL enum natwm_error string_append_char(char **destination,
                                                 char append)
{
        size_t destination_size = strlen(*destination);
        char *tmp = realloc(*destination, destination_size + 2);

        if (tmp == NULL) {
                free(*destination);

                return MEMORY_ALLOCATION_ERROR;
        }

        *destination = tmp;

        (*destination)[destination_size] = append;
        (*destination)[destination_size + 1] = '\0';

        return NO_ERROR;
}

/**
 * Search a string for a specific char
 *
 * Once the char has been found place the index position inside a provided
 * destination pointer
 *
 * If a valid index is found return NO_ERROR otherwise an error will be returned
 */
enum natwm_error string_find_char(const char *haystack, char needle,
                                  size_t *index_dest)
{
        if (haystack == NULL) {
                return INVALID_INPUT_ERROR;
        }

        char ch = '\0';
        size_t index = 0;

        while ((ch = haystack[index]) != '\0' && ch != needle) {
                ++index;
        }

        if (ch == '\0') {
                return NOT_FOUND_ERROR;
        }

        SET_IF_NON_NULL(index_dest, index);

        return NO_ERROR;
}

/**
 * Search a string for the first non whitespace character and set the index
 * of the character into the provided pointer
 *
 * If no non-whitespace characters are found in the supplied string return
 * NOT_FOUND_ERROR otherwise NO_ERROR
 */
enum natwm_error string_find_first_nonspace(const char *string,
                                            size_t *index_result)
{
        if (string == NULL) {
                return INVALID_INPUT_ERROR;
        }

        char ch = '\0';
        size_t index = 0;

        while ((ch = string[index]) != '\0' && isspace(ch)) {
                ++index;
        }

        if (ch == '\0') {
                // Reached the end of the line without finding a nonspace char
                return NOT_FOUND_ERROR;
        }

        *index_result = index;

        return NO_ERROR;
}

/**
 * Search a string for the last non whitespace character and place the index
 * of the character in the provided pointer.
 *
 * If no non-whitespace character is found in the supplied string return
 * NOT_FOUND_ERROR otherwise NO_ERROR
 */
enum natwm_error string_find_last_nonspace(const char *string,
                                           size_t *index_result)
{
        if (string == NULL) {
                return INVALID_INPUT_ERROR;
        }

        char ch = '\0';
        size_t i = 0;
        size_t index = 0;
        bool found = false;

        while ((ch = string[i]) != '\0') {
                if (!isspace(ch)) {
                        index = i;

                        if (!found) {
                                found = true;
                        }
                }

                ++i;
        }

        *index_result = index;

        if (!found) {
                return NOT_FOUND_ERROR;
        }

        return NO_ERROR;
}

/**
 * Gets all characters up to a specified delimiter character
 *
 * The caller must supply a uninitialized char pointer which will
 * contain the newly allocated string, as well as a size pointer which will
 * contain the strlen of the new string
 *
 * The resulting string will consist of all characters up to the delimiter
 * character. If consume is true then the delimiter will also be included
 *
 * The caller must free the result
 *
 * The function either returns NO_ERROR or an error if it occured
 */
enum natwm_error string_get_delimiter(const char *string, char delimiter,
                                      char **destination, size_t *length,
                                      bool consume)
{
        size_t index = 0;
        enum natwm_error find_err = string_find_char(string, delimiter, &index);

        if (find_err != NO_ERROR) {
                return find_err;
        }

        if (consume) {
                ++index;
        }

        enum natwm_error splice_err
                = string_splice(string, 0, index, destination, length);

        if (splice_err != NO_ERROR) {
                return splice_err;
        }

        return NO_ERROR;
}

/**
 * Compare if two strings are equal without considering their case
 *
 * "TWO" == "two"
 * "twO" == "TWo"
 */
bool string_no_case_compare(const char *one, const char *two)
{
        if (one == NULL || two == NULL || (strlen(one) != strlen(two))) {
                return false;
        }

        char ch_one = '\0';
        char ch_two = '\0';

        while ((ch_one = *one) != '\0' && (ch_two = *two) != '\0') {
                if (tolower(ch_one) != tolower(ch_two)) {
                        return false;
                }

                ++one;
                ++two;
        }

        return true;
}

/**
 * Extract a portion of a string into a destination buffer using the supplied
 * start and end indicies.
 *
 * If something is found, space is allocated for the resulting string and a null
 * terminator is added. The caller must free the result.
 *
 * If nothing is found or there was an error then the result will != NO_ERROR
 */
enum natwm_error string_splice(const char *string, size_t start, size_t end,
                               char **destination, size_t *size)
{
        if (string == NULL || end < start || end > strlen(string)) {
                return INVALID_INPUT_ERROR;
        }

        size_t result_size = end - start;

        *destination = malloc(result_size + 1);

        if (*destination == NULL) {
                return MEMORY_ALLOCATION_ERROR;
        }

        memcpy(*destination, string + start, result_size);

        (*destination)[result_size] = '\0';

        SET_IF_NON_NULL(size, result_size);

        return NO_ERROR;
}

enum natwm_error string_split(const char *string, char delimiter,
                              char ***result, size_t *length)
{
        // We will create a list of each item we find, then push them all
        // into an array once we find the total number
        enum natwm_error err = GENERIC_ERROR;
        struct list *found_items = create_list();

        for (;;) {
                char *item = NULL;
                size_t delimiter_pos = 0;
                err = string_get_delimiter(
                        string, delimiter, &item, &delimiter_pos, false);

                if (err == NOT_FOUND_ERROR) {
                        // There are no more delimiters in the string, but we
                        // still need to consume the last item
                        char *last_item = string_init(string);

                        if (last_item == NULL) {
                                goto free_and_error;
                        }

                        list_insert(found_items, last_item);

                        break;
                } else if (err != NO_ERROR) {
                        // We have an actual error
                        goto free_and_error;
                }

                list_insert(found_items, item);

                string += ++delimiter_pos;
        }

        if (list_is_empty(found_items)) {
                return NOT_FOUND_ERROR;
        }

        // We should now have a list of items. We can now make an array of them
        char **items = malloc(sizeof(char *) * found_items->size);

        if (items == NULL) {
                goto free_and_error;
        }

        // Walk the list backwards to store the strings in order
        size_t items_index = (found_items->size - 1);

        LIST_FOR_EACH(found_items, item)
        {
                items[items_index] = (char *)item->data;

                --items_index;
        }

        *result = items;
        *length = found_items->size;

        // We have no need for this list anymore
        destroy_list(found_items);

        return NO_ERROR;

free_and_error:
        // We need to make sure to cleanup any memory allocated while finding
        // items before we return an error
        LIST_FOR_EACH(found_items, item)
        {
                if (item && item->data != NULL) {
                        free((char *)item->data);
                }
        }

        destroy_list(found_items);

        return err;
}

/**
 * Strip the surrounding spaces from a string. The first nonspace and last
 * nonspace characters are found, the source string is then spliced and the
 * resulting characters are allocated into the supplied dest buffer.
 *
 * If there is an error during any step in the process that error is returned
 * to the caller and the dest pointer remains unchanged. If there are no issues
 * the stripped string is placed in the dest pointer and the resulting strlen
 * is placed in the length pointer.
 *
 * If no errors are encountered NO_ERROR is returned
 */
enum natwm_error string_strip_surrounding_spaces(const char *string,
                                                 char **dest, size_t *length)
{
        size_t start = 0;
        size_t end = 0;
        size_t length_result = 0;
        enum natwm_error err = GENERIC_ERROR;

        err = string_find_first_nonspace(string, &start);

        if (err != NO_ERROR) {
                return err;
        }

        err = string_find_last_nonspace(string, &end);

        if (err != NO_ERROR) {
                return err;
        }

        err = string_splice(string, start, end + 1, dest, &length_result);

        SET_IF_NON_NULL(length, length_result);

        return err;
}

/**
 * Takes a string which corresponds to a base 10 number as well as a pointer
 * to intmax_t. if the number can be transformed successfully the dest
 * pointer will have the number placed into it.
 *
 * If there is an error it will be returned. Otherwise NO_ERROR is returned
 */
enum natwm_error string_to_number(const char *number_string, intmax_t *dest)
{
        char *endptr = NULL;

        // Reset errno to 0 so we can test for ERANGE
        errno = 0;

        intmax_t result = strtoimax(number_string, &endptr, 10);

        if (errno == ERANGE || endptr == number_string || *endptr != '\0') {
                return INVALID_INPUT_ERROR;
        }

        if (errno != 0) {
                LOG_CRITICAL_LONG(natwm_logger,
                                  "Unhandleable input received - Exiting...");

                exit(EXIT_FAILURE);
        }

        *dest = result;

        return NO_ERROR;
}
