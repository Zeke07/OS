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
#include <signal.h>

#define WSH_FAULT(msg, code) {perror(msg); exit(code);}
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

// single program - stats
struct Program {
    char **tokens; // program args
    int size; // num args
    int *fd; // [read, write] fds
    int *pip; // all open fds
    int pip_size; // # open fds
    pid_t pgid;
    pid_t pid;
};

// one program chain (one user input line)
struct Pipeline {
    struct Program *progs; // programs in command chain
    int size; // number of programs
    int bg; // if currently a background process
    int init_bg; // if command is spawned with &
};

struct Job {
    struct Pipeline *cmd;
    int valid; // if program is not terminated (all 0 upon init)
};

extern struct Job background[256];
extern struct Job foreground[256];


// built-in commands
int wsh_exit(struct Pipeline *cmd, struct Program *args);
int wsh_jobs(struct Pipeline *cmd, struct Program *args);
//int wsh_fg();
//int wsh_bg();



// shell functions, processing, clean-up
int is_built_in_cmd(struct Program *args);
struct Program process_inputs(char *buffer);
struct Pipeline* process_command(char *buffer);
void wsh_execute(struct Pipeline *c, struct Program *args);
void cmd_pipeline(struct Pipeline *cmd);
void close_fds(int *pipes, int size);
void wsh_clean(struct Pipeline *c);
void add_job(struct Pipeline *cmd, struct Job jobs[256]);
void format_job(char string_builder[256], struct Pipeline *cmd, int id);
void update_job_list(struct Pipeline *cmd, struct Job jobs[256]);
void configure_handlers();

static int (*built_in_cmds[])(struct Pipeline*, struct Program*) = {
        [EXIT] = wsh_exit,
        [JOBS] = wsh_jobs
        //[FG] = wsh_fg,
        //[BG] = wsh_bg,

};

#endif

