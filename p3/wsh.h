/*
 *
 *
 */
#ifndef P3_WSH
#define P3_WSH

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define NO_CMD 0
#define EXIT 1

size_t BUFFER_SIZE = 256;
struct Args {
    char **tokens;
    int size;
};

int wsh_exit(struct Args *args);

static int (*built_in_cmds[])(struct Args*) = {
        [EXIT] = wsh_exit
};

#endif