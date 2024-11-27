#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <X11/X.h>
#include <X11/extensions/Xrandr.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include "config.h"
#include "navigation.h"
#include "util.h"
#include "vec.h"

#define MIN_GLX_MAJOR   1
#define MIN_GLX_MINOR   3

GLuint load_shader(const char *, GLenum);
XImage* get_screenshot();
void button_press(XEvent *);
void button_release(XEvent *);
void check_glx_version(Display *);
void destroy_screenshot(XImage*);
void draw_image(Camera *, XImage *, GLuint, GLuint, Vec2f, Mouse *, Flashlight *);
void keypress(XEvent *);
void motion_notify(XEvent *);
void scroll_down(unsigned int, bool);
void scroll_up(unsigned int, bool);

static Display *dpy = NULL;
static int screen = 0;
static XWindowAttributes wa;
static Window w;
static bool running = true;

static Flashlight flashlight;
static Camera camera;
static Mouse mouse;
static Config config;

static void (*handler[LASTEvent]) (XEvent *) = {
    [MotionNotify] = motion_notify,
    [KeyPress] = keypress,
    [ButtonPress] = button_press,
    [ButtonRelease] = button_release,
};

GLuint
load_shader(const char *name, GLenum type)
{
    FILE *fp = fopen(name, "r");
    GLchar *shaderSrc = NULL;

    if (fp == NULL)
        die("Unable to open shader file at '%s':", name);

    size_t len;
    ssize_t bytes_read = getdelim(&shaderSrc, &len, '\0', fp);

    if (bytes_read < 0)
        die("Unable to read shader file at '%s'.", name);
    fclose(fp);

    GLuint shader;

    shader = glCreateShader(type);

    glShaderSource(shader, 1, (const GLchar **)&shaderSrc, NULL);
    glCompileShader(shader);

    int sucess = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &sucess);
    if (!sucess) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        die("Error comiling shader:\n%s", infoLog);
    }

    return shader;
}

void 
check_glx_version(Display *dpy)
{
    int glx_major, glx_minor;
    // Check shit if necessary
    if (!glXQueryVersion(dpy, &glx_major, &glx_minor)
        || ((glx_minor == MIN_GLX_MAJOR) && (glx_minor < MIN_GLX_MINOR))
        || (MIN_GLX_MAJOR < 1))
        die("Invalid GLX version %d.%d. Requires GLX >= %d.%d", glx_major, glx_minor, MIN_GLX_MAJOR, MIN_GLX_MINOR);
}

// TODO: implement support for the MIT shared memory extension. (MIT-SHM)
XImage*
get_screenshot()
{
    return XGetImage(
        dpy, w,
        0, 0,
        wa.width,
        wa.height,
        AllPlanes,
        ZPixmap);
}

void
destroy_screenshot(XImage *screenshot)
{
    XDestroyImage(screenshot);
}

void
draw_image(Camera *cam, XImage *img, GLuint shader, GLuint vao, Vec2f window_size, Mouse *mouse, Flashlight *fl)
{
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader);

    glUniform2f(glGetUniformLocation(shader, "cameraPos"), cam->position.x, cam->position.y);
    glUniform1f(glGetUniformLocation(shader, "cameraScale"), cam->scale);
    glUniform2f(glGetUniformLocation(shader, "screenshotSize"), (float)img->width, (float)img->height);
    glUniform2f(glGetUniformLocation(shader, "windowSize"), window_size.x, window_size.y);
    glUniform2f(glGetUniformLocation(shader, "cursorPos"), mouse->current.x, mouse->current.y);
    glUniform1f(glGetUniformLocation(shader, "flShadow"), fl->shadow);
    glUniform1f(glGetUniformLocation(shader, "flRadius"), fl->radius);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void 
keypress(XEvent *e) 
{
    KeySym keysym;
    XKeyEvent *ev;

    ev = (XKeyEvent*)&e->xkey;
    keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, e->xkey.state & ShiftMask ? 1 : 0);
    switch (keysym) {
    case XK_Left:
    case XK_h:
        camera.velocity.x -= config.key_move_speed;
        break;
    case XK_Down:
    case XK_j:
        camera.velocity.y += config.key_move_speed;
        break;
    case XK_Up:
    case XK_k:
        camera.velocity.y -= config.key_move_speed;
        break;
    case XK_Right:
    case XK_l:
        camera.velocity.x += config.key_move_speed;
        break;
    case XK_minus:
        scroll_down(1, ev->state & ControlMask);
        break;
    case XK_equal:
        scroll_up(1, ev->state & ControlMask);
        break;
    case XK_q:
    case XK_Escape:
        running = false;
        break;
    case XK_0:
        camera.scale = 1.0f;
        camera.delta_scale = 0.0f;
        camera.velocity = camera.position = (Vec2f) {0, 0};
        break;
    case XK_r:
        config = load_config();
        break;
    case XK_f:
        flashlight.is_enabled = !flashlight.is_enabled;
        break;
    }
}

void
scroll_up(unsigned int delta, bool fl_enabled)
{
    if ((delta > 0) && fl_enabled) {
        flashlight.delta_radius += 250.0f;
    } else {
          camera.delta_scale += config.scroll_speed;
          camera.scale_pivot = mouse.current;
    }
}

void
scroll_down(unsigned int delta, bool fl_enabled)
{
    if ((delta > 0) && fl_enabled) {
        flashlight.delta_radius -= 250.0f;
    } else {
        camera.delta_scale -= config.scroll_speed;
        camera.scale_pivot = mouse.current;
    }
}

void
button_press(XEvent *e)
{
    XButtonEvent *ev;
    
    ev = (XButtonEvent*)&e->xbutton;
    switch (ev->button) {
    case Button1:
        mouse.previous = mouse.current;
        mouse.dragging = true;
        camera.velocity = ZERO;
        break;
    case Button4:
        scroll_up(ev->state & ControlMask, flashlight.is_enabled);
        break;
    case Button5:
        scroll_down(ev->state & ControlMask, flashlight.is_enabled);
        break;
    }
}

void
button_release(XEvent *e)
{
    XButtonEvent *ev;

    ev = (XButtonEvent*)&e->xbutton;
    if (ev->button == Button1)
        mouse.dragging = false;
}

void
motion_notify(XEvent *e)
{
    XMotionEvent *ev;

    ev = (XMotionEvent*)&e->xmotion;

    mouse.current = (Vec2f) {ev->x, ev->y};
    if (mouse.dragging) {
        Vec2f delta = SUB(world(&camera, mouse.previous), world(&camera, mouse.current));
        /* delta is the distance the mouse traveled in a single
         * frame. To turn the velocity into units/second we need to
         * multiple it by FPS.
         */
        camera.position = ADD(camera.position, delta);
        camera.velocity = MULS(delta, camera.dt);
    }
    mouse.previous = mouse.current;
}

int
main(int argc, char *argv[])
{
    config = load_config();

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        die("Cannot connect to the X display server!\n");
    }
    check_glx_version(dpy);

    screen = DefaultScreen(dpy);

    GLint attrs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };

    XVisualInfo *vi = glXChooseVisual(dpy, screen, attrs);
    if (vi == NULL)
        die("No appropriate visual found!\n");

    XSetWindowAttributes swa;
    memset(&swa,0,sizeof(XSetWindowAttributes));

    swa.colormap = XCreateColormap(dpy, DefaultRootWindow(dpy), vi->visual, AllocNone);
    swa.event_mask = ButtonPressMask 
        | ButtonReleaseMask 
        | KeyPressMask 
        | KeyReleaseMask 
        | PointerMotionMask 
        | ClientMessage;

    if (!config.windowed) {
        swa.override_redirect = 1;
        swa.save_under = 1;
    }

    XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &wa);

     w = XCreateWindow(
        dpy, DefaultRootWindow(dpy), 
        0, 0, wa.width, wa.height, 0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask | CWOverrideRedirect | CWSaveUnder, &swa
    );

    XMapWindow(dpy, w);

    char *wm_name  = "zooc";
    char *wm_class = "zooc";
    XClassHint hints = {.res_name = wm_name, .res_class = wm_class};

    XStoreName(dpy, w, wm_name);
    XSetClassHint(dpy, w, &hints);

    Atom wm_delete_atom = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(dpy, w, &wm_delete_atom, 1);

    GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, w, glc);

    if (GLEW_OK != glewInit())
        die("Couldnt initialize glew!\n");

    glViewport(0, 0, wa.width, wa.height);

    XSelectInput(dpy, w, ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask);

    int revert_to_parent;
    Window origin_win;

    XGetInputFocus(dpy, &origin_win, &revert_to_parent);

    GLuint vertex_shader;
    GLuint fragment_shader;

    /* Load and compile shaders */
    vertex_shader   = load_shader(config.vertex_shader_file, GL_VERTEX_SHADER);
    fragment_shader = load_shader(config.fragment_shader_file, GL_FRAGMENT_SHADER);

    /* Link shaders and create a program */
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    int link_success;
    glGetProgramiv(shader_program, GL_LINK_STATUS, &link_success);

    if(!link_success) {
        GLchar info_log[512];
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        die("Error whilst linking program:\n%s", info_log);
    }

    XImage *screenshot = get_screenshot();
    Vec2f screenshot_size = (Vec2f) {screenshot->width, screenshot->height};

    int sw = screenshot_size.x;
    int sh = screenshot_size.y;

    GLuint vbo, vao, ebo;
    GLfloat vertices[] = {
        //x     y   z       UV coords
        sw,     0,  0.0,    1.0, 1.0, /* Top right */
        sw,     sh, 0.0,    1.0, 0.0, /* Bottom right */
        0,      sh, 0.0,    0.0, 0.0, /* Bottom left */
        0,      0,  0.0,    0.0, 1.0  /* Top left */
    };
    /* Indecies of the triangles. 
     * We want to fill a screen rect so we create two triangles:
     * 3_____0
     * |\    |
     * |  \  |
     * 2____\1
     *
     * Therefore we have two triangles, 0-1-3 and 1-2-3.
     */
    GLuint indices[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW); 

    GLsizei stride = 5 * sizeof(GLfloat);

    /* Pos attribute = vec3(x, y, z) */
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    /* UV attribute = vec2(x, y) */
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_RGB, 
        screenshot->width,
        screenshot->height,
        0,
        GL_BGRA,
        GL_UNSIGNED_BYTE,
        screenshot->data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    /* bind tex in the glsl code to be the loaded texture */
    glUniform1i(glGetUniformLocation(shader_program, "tex"), 0);

    glEnable(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    XRRScreenConfiguration *screen_config = XRRGetScreenInfo(dpy, DefaultRootWindow(dpy));
    float rate = 1.0f / XRRConfigCurrentRate(screen_config);

    flashlight = (Flashlight) {
        .is_enabled = false,
        .shadow = 0.0f,
        .radius = 200.0f,
        .delta_radius = 0.0f,
    };

    camera = (Camera) {
        .position = ZERO,
        .velocity = ZERO,
        .scale_pivot = ZERO,
        .scale = 1.0f,
        .delta_scale = 0.0f,
        .dt = rate,
    };

    initialize_mouse(dpy, &mouse);

    XEvent e;
    while (running) {
        // HACK: setting this every time is probably inefficient. Is there a 
        // better way to maintain fullscreen input focus?
        if (!config.windowed)
            XSetInputFocus(dpy, w, RevertToParent, CurrentTime);

        while (XPending(dpy) > 0) {
            XNextEvent(dpy, &e);
            switch (e.type) {
            case ClientMessage:
                if ((Atom)e.xclient.data.l[0] == wm_delete_atom)
                    running = false;
                break;
            case KeyPress:
            case ButtonPress:
            case ButtonRelease:
            case MotionNotify:
                handler[e.type](&e);
                break;
            default:
                break;
            }
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        update_flashlight(&flashlight, camera.dt);
        update_camera(&camera, &config, &mouse, screenshot_size);

        glUseProgram(shader_program);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        draw_image(&camera, screenshot, shader_program, vao, screenshot_size, &mouse, &flashlight);

        glXSwapBuffers(dpy, w);
        glFinish();
    }

    XSetInputFocus(dpy, origin_win, RevertToParent, CurrentTime);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, glc);

    XCloseDisplay(dpy);
    return 0;
}
