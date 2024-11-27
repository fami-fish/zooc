#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

void 
die(const char *fmt, ...)
{
    va_list args;
    int saved_errno;

    saved_errno = errno;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if (fmt[0] && fmt[strlen(fmt)-1] == ':')
        fprintf(stderr, " %s", strerror(saved_errno));
    fputc('\n', stderr);

    exit(EXIT_FAILURE);
}
