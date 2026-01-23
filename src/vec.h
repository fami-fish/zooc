#ifndef ZOOC_VEC_H
#define ZOOC_VEC_H

#include <math.h>

#include <GL/gl.h>

#define UNIT        (Vec2f) {1.0f, 1.0f}
#define ZERO        (Vec2f) {0.0f, 0.0f}

/* Vector operations */
#define MUL(v, u)   (Vec2f) {(v).x * (u).x, (v).y * (u).y}
#define DIV(v, u)   (Vec2f) {(v).x / (u).x, (v).y / (u).y}
#define ADD(v, u)   (Vec2f) {(v).x + (u).x, (v).y + (u).y}
#define SUB(v, u)   (Vec2f) {(v).x - (u).x, (v).y - (u).y}

#define EQ(v, u)    ((v).x == (u).x && (v).y == (u).y)
#define LEN(v)      sqrtf((v).x * (v).x + (v).y * (v).y)
#define NORM(v)     LEN(v) == 0.0f ? ZERO : (DIV(v, LEN(v)))

/* Scalar operations */
#define MULS(v, s)  (Vec2f) {(v).x * (s), (v).y * (s)}
#define DIVS(v, s)  (Vec2f) {(v).x / (s), (v).y / (s)}
#define ADDS(v, s)  (Vec2f) {(v).x + (s), (v).y + (s)}
#define SUBS(v, s)  (Vec2f) {(v).x - (s), (v).y - (s)}

typedef struct {
    GLfloat x;
    GLfloat y;
} Vec2f;

#endif
