#ifndef ZOOC_NAVIGATION_H
#define ZOOC_NAVIGATION_H

#include <stdbool.h>

#include <GL/gl.h>

#include <X11/Xlib.h>

#include "vec.h"
#include "config.h"

typedef struct {
    Vec2f position;
    Vec2f velocity;
    Vec2f scale_pivot;
    GLfloat scale;
    GLfloat delta_scale;
    float dt;
} Camera;

typedef struct {
    bool is_enabled;
    GLfloat shadow;
    GLfloat radius;
    GLfloat delta_radius;
} Flashlight;

typedef struct {
    Vec2f current;
    Vec2f previous;
    bool dragging;
} Mouse;

Vec2f world(Camera *, Vec2f);
void initialize_mouse(Display *, Mouse *);
void update_camera(Camera *, Config *, Mouse *, Vec2f);
void update_flashlight(Flashlight *, float);

#endif
