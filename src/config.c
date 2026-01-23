#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include "config.h"
#include "util.h"

#define MAX_PATH_SIZE 4096

Config get_default_config(void);
void parse_config(Config *, FILE *);
int parse_bool(char *arg);

Config
get_default_config(void)
{
    return (Config){
        .min_scale = 0.1,
        .max_scale = 6.0,
        .drag_friction = 6.0,
        .scale_friction = 4.0,
        .scroll_speed = 1.5,
        .key_move_speed = 400.0,
        .windowed = false,
        .screenshot_path = getenv("HOME"),

        /* Set in code */
        .vertex_shader_file = NULL,
        .fragment_shader_file = NULL,
    };
}

Config
load_config(void)
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

    /* To avoid truncation, we subtract 15 bytes here, allowing for appending
     * different file suffixes without truncation.
     */
    char config_dir[MAX_PATH_SIZE - 15];
    snprintf(config_dir, sizeof(config_dir), "%s/zooc", xdg_config_home);
    if (mkdir(config_dir, 0755) == -1 && errno != EEXIST) {
        if (xdg_config_home && xdg_config_home != getenv("XDG_CONFIG_HOME"))
            free((void *)xdg_config_home);
        die("Failed to create directory '%s': %s:", xdg_config_home);
    }

    char config_file[MAX_PATH_SIZE];
    snprintf(config_file, MAX_PATH_SIZE, "%s/config.conf", config_dir);

    if (access(config_file, F_OK | R_OK)) {
        strncpy(config_file, "/etc/zooc/config.conf", sizeof(config_file));
    }

    FILE *f = fopen(config_file, "r");
    if (f == NULL)
        die("Unable to open config file: %s\n", config_file);

    char *vertex_shader_file   = malloc(MAX_PATH_SIZE);
    char *fragment_shader_file = malloc(MAX_PATH_SIZE);

    snprintf(fragment_shader_file, MAX_PATH_SIZE, "%s/fragment.glsl", config_dir);
    snprintf(vertex_shader_file, MAX_PATH_SIZE, "%s/vertex.glsl", config_dir);

    char message[] = "Unable to open shader file at '%s'.\n%s";
    char hint[]    = "Hint: run 'make install' to create these files.\n";

    if (!fopen(vertex_shader_file, "r")) {
        strncpy(vertex_shader_file, "/etc/zooc/vertex.glsl\0", MAX_PATH_SIZE);
        if (!fopen(vertex_shader_file, "r"))
            die(message, vertex_shader_file, hint);
    }

    if (!fopen(fragment_shader_file, "r")) {
        strncpy(fragment_shader_file, "/etc/zooc/fragment.glsl\0", MAX_PATH_SIZE);
        if (!fopen(fragment_shader_file, "r"))
            die(message, fragment_shader_file, hint);
    }

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

    size_t cur_line = 1;
    while (getline(&line, &len, f) != -1) {
        if (line[0] == '#' || line[0] == '\n') {
            cur_line++;
            continue;
        }
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
            } else if (!strcmp(arg, "screenshot_path")) {
                c[ strlen(c) - 1 ] = '\0';
                conf->screenshot_path = c;
            } else if (!strcmp(arg, "windowed")) {
                if(parse_bool(c) != -1) {
                    conf->windowed = (bool)parse_bool(c);
                }
            } else {
                die("Unexpected configuration key '%s'\n", arg);
            }

            /* get next arg */
            c = strtok(NULL, " \t=");
        }
        cur_line++;
    }

    fclose(f);
}

int
parse_bool(char *arg)
{
    if (arg == NULL)
        return -1;

    /* buff for lowercase string */
    char buff[8] = {0};
    strncpy(buff, arg, sizeof(buff) - 1);

    for (int i = 0; buff[i]; i++) {
        buff[i] = tolower(buff[i]);

        /* remove trailing newline */
        if (buff[i] == '\n' || buff[i] == '\r') {
            buff[i] = '\0';
            break;
        }
    }

    if (!strcmp(buff, "true")
        || !strcmp(buff, "t")
        || !strcmp(buff, "yes")
        || !strcmp(buff, "y")
        || !strcmp(buff, "1"))
        return 1;

    if (!strcmp(buff, "false")
        || !strcmp(buff, "f")
        || !strcmp(buff, "no")
        || !strcmp(buff, "n")
        || !strcmp(buff, "0"))
        return 0;
    return -1;
}
