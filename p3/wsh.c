/*
 *
 *
 */

#include "wsh.h"


struct Job background[256];
struct Job foreground[256];
void format_job(char string_builder[256], struct Pipeline *cmd, int id){


    char *ptr = string_builder;
    snprintf(string_builder, BUFFER_SIZE, "%d: ", id);
    ptr+=3;

    for (int i = 0; i < cmd->size; i++){
        for (int j = 0; j < cmd->progs[i].size-1; j++){ // truncate for padded null pointer

            int length = strlen(cmd->progs[i].tokens[j]);
            if (string_builder + BUFFER_SIZE == string_builder+length+1) // account for padded null-terminator
                SAFE_EXIT("Job string exceeds buffer size\n");

            snprintf(ptr, (int) ((string_builder + BUFFER_SIZE) - ptr), "%s ", cmd->progs[i].tokens[j]);
            ptr+=length+1;
        }
        if (i != cmd->size-1) {
            snprintf(ptr,  (int) ((string_builder + BUFFER_SIZE) - ptr), "| ");
            ptr+=1;
        }

    }

    if (cmd->init_bg) {
        snprintf(ptr,  (int) ((string_builder + BUFFER_SIZE) - ptr), "&");
        ptr+=1;
    }


}


int wsh_exit(struct Pipeline *cmd, struct Program *args){
    SAFE_EXIT(""); // buffer flushed by <enter> on exit call
}

int wsh_jobs(struct Pipeline *cmd, struct Program *args){

    update_job_list(cmd, background);

    int id = 1;
    for (int i = 0; i < BUFFER_SIZE; i++){
        if (background[i].valid) {
            char line[256];
            format_job(line, background[i].cmd, id);
            printf("%s\n", line); // FLUSH THE BUFFER
            id++;
        }
    }

    return 0;
}

/**
 * Update job list - free any stale processes
 * if a job has terminated
 *
 * @param cmd
 * @param jobs
 */
void update_job_list(struct Pipeline *cmd, struct Job jobs[256]){

    for (int i = 0; i < BUFFER_SIZE; i++){

        // evaluate "valid" processes
        if (background[i].valid){

            int status;
            int cpid;
            if (background[i].cmd->size==1)
                cpid = background[i].cmd->progs[0].pid;
            else cpid = background[i].cmd->progs[background[i].cmd->size-1].pid;

            // on kill(9), waitpid will fire -1, check if the pid exists (BUG)


            //if (waitpid(cpid, &status, WNOHANG) < 0) WSH_FAULT("Wait fail on job update\n", -1);
            waitpid(cpid, &status, WNOHANG);

            // rc is 0 on exit
            if (WIFEXITED(status) != 0) {
                background[i].valid = 0;

                // free allocated structure when process has exited()
                wsh_clean(background[i].cmd);
                background[i].cmd = NULL;
            }

        }
    }


}

void add_job(struct Pipeline *cmd, struct Job jobs[256]){

    // check for any completed jobs up to this point
    update_job_list(cmd, background);

    // insert job to the first invalid slot in the array
    int status = 0;
    for (int i = 0; i < BUFFER_SIZE; i++){
        if (!jobs[i].valid) {
            jobs[i].cmd = cmd;
            jobs[i].valid = 1;
            status = 1;
            break;
        }
    }

    // job list is full
    if (!status) SAFE_EXIT("Job could not be appended\n");
}

struct Pipeline* process_command(char *buffer){

    // malloc a Pipeline, init fields
    struct Pipeline *c = malloc(sizeof(struct Pipeline));
    *c = (struct Pipeline) {NULL, 0, 0, 0};

    char *token = NULL;

    char *amp_occ;
    if ((amp_occ = strstr(buffer, "&")) != NULL){
        if (strstr(amp_occ+1, "&") != NULL) {
            SAFE_EXIT("Misplaced '&' token\n");
        }
        else {
            char *i;
            i = amp_occ + 1;
            while (*i != '\0'){
                if (*i != EOF && *i != ' ' && *i != '\n')
                    SAFE_EXIT("Misplaced '&' token\n");
                i++;
            }
        }

        *amp_occ = '\0';
        c->bg = 1;
        c->init_bg = 1;
    }


    while ((token = strsep(&buffer, "|")) != NULL){
        if (strcmp(token, "") != 0){

            // truncate a newline if it exists
            size_t tok_len = strlen(token);

            if (token[tok_len-1] == '\n')
                token[tok_len-1] = '\0';

            // SEG FAULT HERE AFTER kill -9, null out ptr
            c->progs = realloc(c->progs, sizeof(struct Program) * (c->size + 1));
            c->progs[c->size++] = process_inputs(token);
        }
    }

    return c;
}

struct Program process_inputs(char *buffer){

    char **tokens = NULL;
    char *token = NULL;
    int tok_size = 0;

    while((token = strsep(&buffer, " ")) != NULL){

        if (strcmp(token, "") != 0 && strcmp(token, "\n") != 0) { // ignore empty and newline tokens

            // truncate a newline if it exists
            size_t tok_len = strlen(token);
            if (token[tok_len-1] == '\n') token[tok_len-1] = '\0';

            tokens = realloc(tokens, sizeof(char *) * (tok_size + 1));
            tokens[tok_size] = malloc(sizeof(char) * strlen(token));
            strcpy(tokens[tok_size], token);
            tok_size++;
        }
    }

    // user program case: allocate null-pointer for execv arguments, include this in the size for
    // further processing
    tokens = realloc(tokens, sizeof(char*) * (tok_size+1));
    tokens[tok_size++] = NULL;

    struct Program t = {tokens, tok_size};
    return t;

}

int is_built_in_cmd(struct Program *args){

    if (args->size == 2 && strcmp(args->tokens[0],"exit")==0)
            return EXIT;
    if (args->size == 2 && strcmp(args->tokens[0],"jobs")==0)
        return JOBS;


    return NO_CMD;
}
void wsh_execute(struct Pipeline *c, struct Program *args){

    // validate program - check if it is either a user program or built-in command
    char path[BUFFER_SIZE];
    snprintf(path, BUFFER_SIZE, "/usr/bin/%s", args->tokens[0]);
    int cmd_mode = is_built_in_cmd(args);

    // execute new child process
    pid_t pid = fork();

    if (pid < 0) {
        WSH_FAULT("Failed to spawn process on fork\n", -1);
    } else if (pid == 0) { // CHILD

        // configure file-descriptors with stdin/out
        if (args->fd[0] != STDIN_FILENO)
            dup2(args->fd[0], STDIN_FILENO);

        if (args->fd[1] != STDOUT_FILENO)
            dup2(args->fd[1], STDOUT_FILENO);


        close_fds(args->pip, args->pip_size);


        // execute program
        // case: built-in command --> invoke function and exit
        if (cmd_mode != NO_CMD) {
            built_in_cmds[cmd_mode](c, args);
            exit(0);
        }
        // case: linux program --> call exec, fail if child moves past it
        else execvp(args->tokens[0], args->tokens);
        WSH_FAULT("Failed to execute program\n", -1);
    }

    // PARENT: establish process/group ids of the spawned child
    args->pid = pid; // record pid of current process
    pid_t pgid; // for setting group id of the current process



    // set pgid based on background bit, else pgid shared with parent
    if (c->bg){


        // if this program is the first in a chain, set group id to its own pid
        if (args==c->progs) pgid = pid;

        // else, the first (lead) of the group is already initialized
        // set any successive programs in the chain to this pgid
        else pgid = c->progs[0].pgid;

    }
    else pgid = getpid();

    // set group id of process
    // record group id in struct
    setpgid(pid, pgid);
    args->pgid = pgid;

}

void wsh_clean(struct Pipeline *c){
    for (int i = 0; i < c->size; i++) {

        // free each program arg set
        for (int j = 0; j < c->progs[i].size - 1; j++) // NULL-ptr padding
            free(c->progs[i].tokens[j]);
        free(c->progs[i].tokens);
    }
    free(c->progs); // free process structs on heap
    free(c); // free pipeline struct
}

void close_fds(int *fds, int size){
    for (int i = 0; i < size; i++){
        if (fds[i] != STDIN_FILENO && fds[i] != STDOUT_FILENO)
            close(fds[i]);
    }
}

void cmd_pipeline(struct Pipeline *cmd){

    // load any file descriptors necessary to run the command pipeline
    int n_descriptors = ((cmd->size-1)*2)+2;
    int pipes[n_descriptors];
    pipes[0] = STDIN_FILENO;
    pipes[n_descriptors-1] = STDOUT_FILENO;
    for (int i = 1; i < n_descriptors-1; i+=2){

        int fd[2];
        if (pipe(fd) < 0)
            WSH_FAULT("failed to create pipe\n", -1);

        pipes[i] = fd[1]; // write end
        pipes[i+1] = fd[0]; // read end

    }

    // exec progs
    for (int i = 0; i < cmd->size; i++) {

        cmd->progs[i].fd = pipes+(i*2);
        cmd->progs[i].pip = pipes;
        cmd->progs[i].pip_size = n_descriptors;
        wsh_execute(cmd, cmd->progs+i);

    }


    // close all file descriptors for clean-up
    close_fds(pipes, n_descriptors);

    // used to ensure waits are not issued on built_ins while background processes in op (BUG REPAIR)

    // wait on all children in the foreground
    if (!cmd->bg) {
        add_job(cmd, foreground); // add to foreground structure
        for (int i = 0; i < cmd->size; i++) {
            int status; // change later, check output
            waitpid(-1, &status, WUNTRACED); // WUNTRACED, WNOHANG
        }
    } else add_job(cmd, background); // case: job is in the background (post exec wait)



}

void configure_handlers(){

}

int main(int argc, char **argv){
    int mode = INTERACTIVE; // default is interactive mode


    // swap to batch processing if execute has an arg
    FILE *fp = NULL;
    if (argc > 1) {
        mode = BATCH;
        fp = freopen(argv[1], "r", stdin);
        if (fp == NULL) WSH_FAULT("Null file pointer\n", -1);
    }

    // NOTE: things to add to clean --> free any mallocs on exit, close any open files
    char *buffer; // <NULL>

    while (1){

        if (mode) printf("wsh> "); // check if interactive mode (1)

        // EOF CASE - CTRL + D
        if (feof(stdin)) {
            if (!mode) {
                fclose(fp); // Free allocation to batch file if applicable
                SAFE_EXIT("") // buffer flushed by last exec call
            }
            SAFE_EXIT("\n");
        }

        if (getline(&buffer, &BUFFER_SIZE, stdin) >= 0) {
            // if stdin stream is non-empty, process any inputs
            if (strlen(buffer) > 0) {

                // load all program calls/pipes by the user into command-chain struct

                // ENORMOUS BUG: things are getting set to stack pointer that gets destroyed! MALLOC THE CMD
                struct Pipeline *c = process_command(buffer);

                // execute command chain from user input
                cmd_pipeline(c);


            }
        }
    }

}

