// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "constants.h"
#include "util.h"

/*
 * Duplicates a stack allocated string to a heap allocated string. This allows
 * for mutating/building upon a string with for instance the string_append
 * function
 */
ATTR_NONNULL char *alloc_string(const char *string)
{
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
int string_append(char **destination, const char *append)
{
        char *tmp;
        size_t destination_size = strlen(*destination);
        size_t append_size = strlen(append);
        size_t result_size = destination_size + append_size;

        tmp = realloc(*destination, result_size + 1);

        if (tmp == NULL) {
                free(*destination);

                return -1;
        }

        *destination = tmp;

        strncpy(*destination + destination_size, append, append_size);

        (*destination)[result_size] = '\0';

        return 0;
}

/*
 * Takes a pointer to a heap allocated string and appends a single char to the
 * end of it, making sure to add a null terminator to the end. The resulting
 * string is reallocated in place of the first argument, so it must be freed
 */
int string_append_char(char **destination, char append)
{
        char *tmp;
        size_t destination_size = strlen(*destination);

        tmp = realloc(*destination, destination_size + 2);

        if (tmp == NULL) {
                free(*destination);

                return -1;
        }

        *destination = tmp;

        (*destination)[destination_size] = append;
        (*destination)[destination_size + 1] = '\0';

        return 0;
}

/**
 * Search a string for a specific char
 *
 * Once the char has been found return the index position of it
 */
ssize_t string_find_char(const char *haystack, char needle)
{
        char ch;
        ssize_t index = 0;

        if (haystack == NULL) {
                return -1;
        }

        while ((ch = haystack[index]) != '\0' && ch != needle) {
                ++index;
        }

        if (ch == '\0') {
                return -1;
        }

        return index;
}

/**
 * Search a string for the first non whitespace character and return the
 * index of the character to the caller
 *
 * If no non-whitespace characters are found in the supplied string return -1
 */
ssize_t string_find_first_nonspace(const char *string)
{
        if (string == NULL) {
                return -1;
        }

        ssize_t index = 0;
        char ch;

        while ((ch = string[index]) != '\0' && isspace(ch)) {
                ++index;
        }

        if (ch == '\0') {
                // Reached the end of the line without finding a nonspace char
                return -1;
        }

        return index;
}

/**
 * Search a string for the last non whitespace character and return the index
 * of the character to the caller.
 *
 * If no non-whitespace character is found in the supplied string return -1
 */
ssize_t string_find_last_nonspace(const char *string)
{
        if (string == NULL) {
                return -1;
        }

        ssize_t index = (ssize_t)strlen(string) - 1;

        while (index >= 0 && isspace(string[index])) {
                --index;
        }

        // If we reach the end of the string we will already be at -1
        return index;
}

/**
 * Gets all characters up to a specified delimiter character
 *
 * The caller must supply a uninitialized char pointer which will
 * contain the newly allocated string.
 *
 * The caller must free the result
 *
 * This function returns the strlen of the new string or -1 if there was
 * an error
 */
ssize_t string_get_delimiter(const char *source, char delimiter,
                             char **destination, bool consume)
{
        ssize_t delimiter_index = string_find_char(source, delimiter);

        if (delimiter_index < 0) {
                return -1;
        }

        if (consume) {
                // Include the delimiter as well
                delimiter_index++;
        }

        char *buffer = malloc((size_t)delimiter_index + 1);

        if (buffer == NULL) {
                return -1;
        }

        *destination = buffer;

        memcpy(*destination, source, (size_t)delimiter_index);

        (*destination)[(size_t)delimiter_index] = '\0';

        return delimiter_index;
}

/**
 * Get size of a file
 *
 * return -1 if we can't find the size
 */
ssize_t get_file_size(FILE *file)
{
        fseek(file, 0L, SEEK_END);

        ssize_t size = ftell(file);

        if (size < 0) {
                return -1;
        }

        fseek(file, 0L, SEEK_SET);

        return size;
}

/*
 * Returns whether or not a path exists
 */
bool path_exists(const char *path)
{
        if (access(path, F_OK) != 0) {
                return false;
        }

        return true;
}
