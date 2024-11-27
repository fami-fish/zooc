#ifndef ZOOC_CONFIG_H
#define ZOOC_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    float min_scale;
    float max_scale;
    float drag_friction;
    float scale_friction;
    float scroll_speed;
    float key_move_speed;
    bool windowed;

    char *fragment_shader_file;
    char *vertex_shader_file;
} Config;

Config load_config();
void write_default_config(FILE*);

#endif
