#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "util.h"

#define MAX_PATH_SIZE 4096

Config get_default_config();
void parse_config(Config *, FILE *);
int parse_bool(char *arg);

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
        .windowed = false,

        /* Set in code */
        .vertex_shader_file = NULL,
        .fragment_shader_file = NULL,
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
load_config()
{
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home == NULL) {
        const char *home = getenv("HOME");
        if (home == NULL)
            die("Error: HOME environment variable not set!\n");

        xdg_config_home = malloc(strlen(home) + strlen("/.config") + 1);
        if (xdg_config_home == NULL)
            die("Malloc failed to allocate:");

        /* malloc already is guaranteed to return enough bytes, so no snprintf
         * is needed
         */
        sprintf((char *)xdg_config_home, "%s/.config", home);
    }

    /* Create app-specific config directory */
    char config_dir[MAX_PATH_SIZE];
    snprintf(config_dir, sizeof(config_dir), "%s/zooc", xdg_config_home);
    if (mkdir(config_dir, 0755) == -1 && errno != EEXIST) {
        if (xdg_config_home && xdg_config_home != getenv("XDG_CONFIG_HOME"))
            free((void *)xdg_config_home);
        die("Failed to create directory '%s': %s:", xdg_config_home);
    }

    char config_file[MAX_PATH_SIZE];
    snprintf(config_file, MAX_PATH_SIZE, "%s/config.conf", config_dir);

    FILE *f = fopen(config_file, "r");

    if (f == NULL) {
        f = fopen(config_file, "wr");
        if (f == NULL)
            die("Unable to open config file %s for writing:", config_file);
        write_default_config(f);
    }

    char *vertex_shader_file = (char *)malloc(MAX_PATH_SIZE);
    char *fragment_shader_file = (char *)malloc(MAX_PATH_SIZE);

    snprintf(fragment_shader_file, MAX_PATH_SIZE, "%s/fragment.glsl", config_dir);
    snprintf(vertex_shader_file, MAX_PATH_SIZE, "%s/vertex.glsl", config_dir);

    char message[] = "Unable to open shader file at '%s'.\n";
    char hint[]    = "Hint: run 'make install' to create these files.\n";

    if (!fopen(vertex_shader_file, "r"))
        die(message, vertex_shader_file, hint);

    if (!fopen(fragment_shader_file, "r"))
        die(message, fragment_shader_file, hint);

    Config conf = get_default_config();
    conf.fragment_shader_file = fragment_shader_file;
    conf.vertex_shader_file = vertex_shader_file;

    parse_config(&conf, f);
    return conf;
}

void
parse_config(Config *conf, FILE *f)
{
    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, f) != -1) {
        char *c;
        c = strtok(line, " \t=");
        while (c != NULL) {
            /* c is argument */
            char *arg = c;

            /* Here, we ignore tabs spaces and equals. Equals is optional in
             * the config and isn't actually required. This is deliberate so 
             * users can format as they like (though the default uses =)
             */
            c = strtok(NULL, " \t=");
            if (c == NULL)
                die(NULL, "Expected value for argument %s\n", arg);

            if (!strcmp(arg, "min_scale")) {
                conf->min_scale = strtof(c, NULL);
            } else if (!strcmp(arg, "max_scale")) {
                conf->max_scale = strtof(c, NULL);
            } else if (!strcmp(arg, "scroll_speed")) {
                conf->scroll_speed = strtof(c, NULL);
            } else if (!strcmp(arg, "drag_friction")) {
                conf->drag_friction = strtof(c, NULL);
            } else if (!strcmp(arg, "scale_friction")) {
                conf->scale_friction = strtof(c, NULL);
            } else if (!strcmp(arg, "key_move_speed")) {
                conf->key_move_speed = strtof(c, NULL);
            } else if (!strcmp(arg, "windowed")) {
                if(parse_bool(c) != -1)
                    conf->windowed = (bool)parse_bool(c);
            } else {
                die("Unexpected configuration key '%s'\n", arg);
            }

            /* get next arg */
            c = strtok(NULL, " \t=");
        }
    }

    fclose(f);
}

int
parse_bool(char *arg)
{
    if (arg == NULL)
        return -1;

    /* buff for lowercase string */
    char buff[8];
    strncpy(buff, arg, sizeof(buff) -1);
    buff[sizeof(buff) - 1] = '\0';

    for (; *arg; arg++)
        *arg = tolower((char)*arg);

    if (!strcmp(buff, "true") || !strcmp(buff, "1") || !strcmp(buff, "f"))
        return 1;

    if (!strcmp(buff, "false") || !strcmp(buff, "1") || !strcmp(buff, "f"))
        return 0;
}
