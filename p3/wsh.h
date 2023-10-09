/*
 * Definitions for WSH Unix Shell
 *
 * zkhan
 * zakhan5@wisc.edu
 * 9083145236
 * Files: wsh.h, wsh.c
 */
#ifndef P3_WSH
#define P3_WSH

// imports for syscalls, string manipulation,
// terminal control management, signal handling, etc
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


// shell modes
#define BATCH 0
#define INTERACTIVE 1

// built-in command codes
#define NO_CMD 0
#define EXIT 1
#define JOBS 2
#define FG 3
#define BG 4
#define CD 5

// universal buffer size - input/job size
size_t BUFFER_SIZE = 256;

// single program - stats
struct Program {
    char **tokens; // program args
    int size; // num args
    int *fd; // [read, write] fds
    int *pip; // all open fds
    int pip_size; // # open fds
    pid_t pid; // process id
};

// one program chain (one user input line)
struct Pipeline {
    struct Program *progs; // programs in command chain
    int size; // number of programs
    int bg; // background status
    pid_t pgid; // process group id
    char usr_input[256]; // user input buffer for job listings
};

// job structure for a single slot
struct Job {
    struct Pipeline *cmd; // command pipeline associated with this slot
    int valid; // if program is not terminated (all 0 upon init)
};


// structure for maintaining all in-progress jobs
struct Job jobs[256];

/**
 * Custom error thrown to
 *
 * @param msg message to print to stderr
 * @param error_code
 */
void wsh_fault(char *msg, int error_code);


// built-in commands
// exit, jobs, fg, bg, cd

/**
 * Shell exit command
 *
 * @param cmd
 * @param args
 * @return
 */
void wsh_exit(struct Pipeline *cmd, struct Program *args);

/**
 * Jobs command -
 * list any working background jobs
 *
 * @return status code
 */
void wsh_jobs();

/**
 * fg built-in command -
 * swap any background job to the foreground
 * job <id> may be specified, else take latest job
 *
 * @param cmd pipeline struct associated with this program call
 * @param args program structure that holds args
 * @return status code
 */
void wsh_fg();

/**
 * bg built-in command -
 * continue any programs suspended in the background
 * job <id> may be specified, else take latest job
 *
 * @param cmd pipeline struct associated with this program call
 * @param args program structure that holds args
 * @return
 */
void wsh_bg();

/**
 * Linux CD program built-in
 *
 * @param cmd pipeline struct associated with this program call
 * @param args program struct that holds args
 * @return status of call, 0 if success, -1 on error
 */
void wsh_cd();

// shell functions, processing, clean-up
/**
 * String-formatter for jobs printing
 *
 * @param string_builder the token to construct
 * @param job the job whose buffer to format
 * @param id the job id to list
 */
void format_job(char string_builder[256], struct Job *job, int id);

/**
 * Fetch the latest valid background job in the maintained structure
 *
 * @return the id of the latest valid job
 */
int fetch_id();

/**
 * add a new job to the internal structure in the next available slot
 *
 * @param cmd Pipeline struct to be ascribed to a job slot
 */
void add_job(struct Pipeline *cmd);

/**
 * Check if a valid job is stale, evict it if so
 *
 * @param job the job whose status must be evaluated
 */
void job_status(struct Job *job);

/**
 * Generate a Pipeline struct, which contains n programs and their arguments
 * from a one user input to the shell
 *
 * @param buffer stores one line of user input
 * @return a Pipeline struct with n programs
 */
struct Pipeline* process_command(char *buffer);

/**
 * Convert one buffer item, representing program arguments, into a Program struct
 *
 * @param buffer program arguments to be cleaned/parsed
 * @return one heap-allocated Program struct
 */
struct Program process_inputs(char *buffer);

/**
 * Check for existence of a built-in commands
 *
 * @param args user program arguments to be assessed
 * @return a macro that maps to the built-in function, 0 if the entry is not a built-in
 */
int is_built_in_cmd(struct Program *args);

/**
 * Execute a specified user program or built-in command with its respective file descriptors
 *
 * @param c Pipeline structure, we will update pgid of the command stream as needed
 * @param args pointer to the program in the stream to execute
 * @return 0 if command was a built-in (no job structure maintenance), else 1
 */
int wsh_execute(struct Pipeline *c, struct Program *args);

/**
 * Free any known heap-allocated pointers contained within
 * a Pipeline struct
 *
 * @param c the Pipeline struct to be cleaned up
 */
void wsh_clean(struct Pipeline *c);

/**
 * Close the indicated list of file descriptors
 *
 * @param fds list of file descriptors
 * @param size number of file descriptors
 */
void close_fds(int *pipes, int size);

/**
 * Execute a chain of commands
 *
 * @param cmd the command chain structure
 */
void cmd_pipeline(struct Pipeline *cmd);

// signal handlers, terminal control
// handlers are empty to ignore SIGTSTP and SIGINT behavior in the parent
void sigint_handler();
void sigtstp_handler();

/**
 * Bind/block any handlers (SIGINT, SIGTSTP, SIGTTOU)
 */
void configure_handlers();

// look-up table for built-in commands
// macro numbers are indices to a list of function pointers
// for each command
static void (*built_in_cmds[])(struct Pipeline*, struct Program*) = {
        [EXIT] = wsh_exit,
        [JOBS] = wsh_jobs,
        [FG] = wsh_fg,
        [BG] = wsh_bg,
        [CD] = wsh_cd
};

#endif

