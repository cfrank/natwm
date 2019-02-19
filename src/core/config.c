// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <common/constants.h>
#include <common/util.h>
#include "config.h"

static FILE *open_config_file(const char *path)
{
        FILE *file;
        if (path != NULL) {
                file = fopen(path, "r");

                if (file == NULL) {
                        return NULL;
                }

                return file;
        } else {
                struct passwd *db = getpwuid(getuid());

                if (db == NULL) {
                        return NULL;
                }

                char *config_path = alloc_string(db->pw_dir);

                string_append(&config_path, "/.config");
                string_append_char(&config_path, '/');
                string_append(&config_path, NATWM_CONFIG_FILE);

                file = fopen(config_path, "r");

                if (file == NULL) {
                        free(config_path);
                        return NULL;
                }

                free(config_path);
                return file;
        }
}

void initialize_config(const char *path)
{
        FILE *config_file = open_config_file(path);

        if (config_file != NULL) {
                fclose(config_file);
        }
}
