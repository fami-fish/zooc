#ifndef ZOOC_UTIL_H
#define ZOOC_UTIL_H

#define MAX(A, B)        (((A) > (B)) ? (A) : (B))
#define MIN(A, B)        (((A) < (B)) ? (A) : (B))
#define BETWEEN(X, A, B) ((A) <= (X) && (X) <= (B))
#define LENGTH(X)        (sizeof (X) / sizeof(X)[0])
/* A < X < B */
#define CLAMP(A, X, B)   (((X) < (A)) ? (A) : ((B) < (X)) ? (B) : (X))

void die(const char *fmt, ...);

#endif
