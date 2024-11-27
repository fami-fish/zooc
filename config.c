#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

Config get_default_config();

Config
get_default_config()
{
    return (Config){
        .min_scale = 0.1,
        .max_scale = 6.0,
        .drag_friction = 6.0,
        .scale_friction = 4.0,
        .scroll_speed = 1.5,
        .key_move_speed = 400.0,
        .windowed = false
    };
}

void
write_default_config(FILE *f)
{
    fprintf(f, "min_scale      = 0.5\n");
    fprintf(f, "max_scale      = 6.0\n");
    fprintf(f, "scroll_speed   = 1.5\n");
    fprintf(f, "drag_friction  = 6.0\n");
    fprintf(f, "scale_friction = 4.0\n");
    fprintf(f, "key_move_speed = 400.0\n");
    fprintf(f, "windowed       = false\n");
}

Config
load_config(const char *fileName)
{
    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        f = fopen(fileName, "wr");
        if (f == NULL)
            die("Unable to open config file %s\n", fileName);
        write_default_config(f);
    }

    char *line = NULL;
    size_t len = 0;

    Config conf = get_default_config();
    while (getline(&line, &len, f) != -1) {
        char *c;
        c = strtok(line, " \t=");
        while (c != NULL) {
            /* c is argument */
            char *arg = c;

            c = strtok(NULL, " \t=");
            if (c == NULL)
                die(NULL, "Expected value for argument %s\n", arg);

            if (!strcmp(arg, "min_scale")) {
                conf.min_scale = strtof(c, NULL);
            } else if (!strcmp(arg, "max_scale")) {
                conf.max_scale = strtof(c, NULL);
            } else if (!strcmp(arg, "scroll_speed")) {
                conf.scroll_speed = strtof(c, NULL);
            } else if (!strcmp(arg, "drag_friction")) {
                conf.drag_friction = strtof(c, NULL);
            } else if (!strcmp(arg, "scale_friction")) {
                conf.scale_friction = strtof(c, NULL);
            } else if (!strcmp(arg, "key_move_speed")) {
                conf.key_move_speed = strtof(c, NULL);
            } else if (!strcmp(arg, "windowed")) {
                switch (c[0]) {
                case 'f':
                case 'F':
                case 'n':
                case 'N':
                case '0':
                    conf.windowed = false;
                    break;
                case 't':
                case 'T':
                case 'y':
                case 'Y':
                case '1':
                    conf.windowed = true;
                    break;
                default:
                    break;
                }
            } else {
                die("Unexpected configuration key '%s'\n", arg);
            }

            /* get next arg */
            c = strtok(NULL, " \t=");
        }
    }

    fclose(f);
    return conf;
}
