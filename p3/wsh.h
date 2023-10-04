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
#define SAFE_EXIT(msg) {printf("%s", msg); exit(0);}

// shell modes
#define BATCH 0
#define INTERACTIVE 1

// built-in command codes
#define NO_CMD 0
#define EXIT 1
#define FG 2
#define BG 3
#define JOBS 4

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
    int bg;
};

// built-in commands
int wsh_exit(struct Prog *args);
//int wsh_fg();
//int wsh_bg();
//int wsh_jobs();


// shell functions
int is_built_in_cmd(struct Prog *args);
struct Prog process_inputs(char *buffer);
void wsh_execute(struct Prog *args);
void cmd_pipeline(struct CMD *cmd);
void close_fds(int *pipes, int size);
void wsh_clean(struct CMD *c);

static int (*built_in_cmds[])(struct Prog*) = {
        [EXIT] = wsh_exit //,
        //[FG] = wsh_fg,
        //[BG] = wsh_bg,
        //[JOBS] = wsh_jobs,
};

#endif

