#include <X11/Xlib.h>
#include <math.h>

#include "navigation.h"
#include "config.h"
#include "util.h"
#include "vec.h"

/* TODO: replace these with configurable constants */
#define VELOCITY_THRESHOLD  15.0f
#define DELTA_RADIUS_DECEL  10.0f
#define SHADOW_ACCEL        6.0f
#define MAX_SHADOW          0.8f

void
update_flashlight(Flashlight *fl, float dt)
{
    if (fabsf(fl->delta_radius) > 1.0f) {
        fl->radius = MAX(0.0f, fl->radius + fl->delta_radius * dt);
        fl->delta_radius -= fl->delta_radius * DELTA_RADIUS_DECEL * dt;
    }

    /* Smoothly interpolate between on/off */
    if (fl->is_enabled)
        fl->shadow = MIN(fl->shadow + SHADOW_ACCEL * dt , MAX_SHADOW);
    else
        fl->shadow = MAX(fl->shadow - SHADOW_ACCEL * dt , 0.0f);
}

void
initialize_mouse(Display *dpy, Mouse *m)
{
    /* XQueryPointer doesnt accept NULL pointers sadly, so we are forced to 
     * alloc these variables despite never using them.
     */
    Window _root, _child;
    int _win_x, _win_y;
    unsigned int _mask;

    int root_x, root_y;
    /* Initialize in case XQueryPointer fails */
    root_x = root_y = 0;

    XQueryPointer(
        dpy,
        DefaultRootWindow(dpy),
        &_root, &_child,
        &root_x, &root_y,
        &_win_x, &_win_y,
        &_mask
    );

    m->current = m->previous = (Vec2f) {root_x, root_y};
    m->dragging = false;
}

void
update_camera(Camera *cam, Config *config, Mouse *mouse, Vec2f window_size)
{
    if (fabsf(cam->delta_scale) > 0.5f) {
        Vec2f p0 = DIVS(SUB(cam->scale_pivot, MULS(window_size, 0.5)), cam->scale);
        cam->scale = CLAMP(config->min_scale, cam->scale + cam->delta_scale * cam->dt, config->max_scale);
        Vec2f p1 = DIVS(SUB(cam->scale_pivot, MULS(window_size, 0.5)), cam->scale);
        cam->position = ADD(cam->position, SUB(p0, p1));

        cam->delta_scale -= cam->delta_scale * cam->dt * config->scale_friction;
    }

    if (!mouse->dragging && LEN(cam->velocity) > VELOCITY_THRESHOLD) {
        cam->position = ADD(cam->position, MULS(cam->velocity, cam->dt));
        cam->velocity = SUB(cam->velocity, MULS(cam->velocity, cam->dt * config->drag_friction));
    }
}

Vec2f
world(Camera *cam, Vec2f v)
{
    return DIVS(v, cam->scale);
}
