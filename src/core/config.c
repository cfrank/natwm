// Copyright 2019 Chris Frank
// Licensed under BSD-3-Clause
// Refer to the license.txt file included in the root of the project

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <common/constants.h>
#include <common/logger.h>
#include <common/util.h>
#include "config.h"

static FILE *open_config_file(const char *path)
{
        FILE *file;

        if (path != NULL) {
                file = fopen(path, "r");

                if (file == NULL) {
                        LOG_ERROR_SHORT(
                                natwm_logger,
                                "Failed to find configuration file at %s",
                                path);

                        return NULL;
                }

                return file;
        }

        struct passwd *db = getpwuid(getuid());

        if (db == NULL) {
                LOG_ERROR_SHORT(natwm_logger, "Failed to find HOME directory");

                return NULL;
        }

        char *config_path = alloc_string(db->pw_dir);

        string_append(&config_path, "/.config/");
        string_append(&config_path, NATWM_CONFIG_FILE);

        // Check if the file exists
        if (!path_exists(config_path)) {
                LOG_ERROR_SHORT(natwm_logger,
                                "Failed to find configuration file at %s",
                                config_path);

                goto release_config_path_and_error;
        }

        file = fopen(config_path, "r");

        if (file == NULL) {
                goto release_config_path_and_error;
        }

        free(config_path);

        return file;

release_config_path_and_error:
        free(config_path);

        return NULL;
}

void initialize_config(const char *path)
{
        FILE *config_file = open_config_file(path);

        if (config_file == NULL) {
                return;
        }

        fclose(config_file);
}
