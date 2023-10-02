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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


#define FAIL(msg, code) {perror(msg); exit(code);}
#define NO_CMD 0
#define EXIT 1

size_t BUFFER_SIZE = 256;
struct Prog {
    char **tokens;
    int size;
    int *fd; // length 2
    int *pip;
    int pip_size;
};

struct CMD {
    struct Prog *progs;
    int size;
};

// built-in commands
int wsh_exit(struct Prog *args);


// shell functions
int is_built_in_cmd(struct Prog *args);
struct Prog process_inputs(char *buffer);
void wsh_execute(struct Prog *args);
void cmd_pipeline(struct CMD *cmd);
void close_fds(int *pipes, int size);


static int (*built_in_cmds[])(struct Prog*) = {
        [EXIT] = wsh_exit
};

#endif

