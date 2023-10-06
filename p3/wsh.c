/*
 *
 *
 */

#include "wsh.h"


struct Job background[256];
struct Job foreground[256];

void format_job(char string_builder[256], struct Job *job, int id){

    char *ptr = string_builder;
    snprintf(string_builder, BUFFER_SIZE, "%d: ", id);
    ptr+=3;
    snprintf(ptr, BUFFER_SIZE, "%s", job->usr_input);
    int len = strlen(job->usr_input);

    ptr+=len;

    if (job->init_bg) snprintf(ptr, BUFFER_SIZE," &");

}


int wsh_exit(struct Pipeline *cmd, struct Program *args){
    SAFE_EXIT(""); // buffer flushed by <enter> on exit call
}

int wsh_jobs(){

    for (int i = 0; i < BUFFER_SIZE; i++){

        if (background[i].valid) {
            char line[BUFFER_SIZE];
            format_job(line, background+i, i+1);
            printf("%s\n", line);
        }
    }

    return 0;
}

int fetch_id(struct Job *jobs, char mode){
    for (int i = BUFFER_SIZE-1; i >= 0; i--) {
        if (mode == 'f' && jobs[i].valid)
            return i;

    }
    return -1;
}

// USE TSET, SIGNALS NOT RECEIVED
int wsh_fg(struct Pipeline *cmd, struct Program *args) {


    int id;
    if (args->size < 3) id = fetch_id(background, 'f');
    else id = atoi(args->tokens[1]) - 1;
    //printf("ID FOUND: (%d)", id);
    if (id >= 0 && id < BUFFER_SIZE) {

        killpg(background[id].cmd->pgid, SIGCONT);
        add_job(background[id].cmd, foreground, 0);
        background[id].valid = 0;
        //printf("Call on: %d", background[id].cmd->pgid);
        for (int i = 0; i < background[id].cmd->size; i++) {
            int status;
            waitpid(background[id].cmd->progs[i].pid, &status, WUNTRACED); // WUNTRACED, WNOHANG, use pid of each child!
        }

    }
    return 0;
}
int wsh_bg(struct Pipeline *cmd, struct Program *args){

    int id;
    if (args->size < 3) id = fetch_id(background, 'f');
    else id = atoi(args->tokens[1]) - 1;
    //printf("ID FOUND: (%d)", id);
    if (id >= 0 && id < BUFFER_SIZE)
        killpg(background[id].cmd->pgid, SIGCONT);

    return 0;
}

int wsh_cd(struct Pipeline *cmd, struct Program *args){
    if (args->size == 3){
        int status = chdir(args->tokens[1]);
        if (status < 0) SAFE_EXIT("Bad filepath: cd\n")

        return 0;
    }
    SAFE_EXIT("Poor usage: cd <filepath>\n");
}

void add_job(struct Pipeline *cmd, struct Job *jobs, int susp){


    // insert job to the first invalid slot in the array
    int status = 0;
    for (int i = 0; i < BUFFER_SIZE; i++){
        if (!jobs[i].valid) {

            jobs[i].cmd = cmd;
            jobs[i].valid = 1;
            jobs[i].init_bg = cmd->init_bg;
            jobs[i].suspended = susp;
            strncpy(jobs[i].usr_input, cmd->usr_input, BUFFER_SIZE);
           // printf("Job added: (%s,%d, %d)\n", jobs[i].usr_input, jobs[i].valid, jobs[i].suspended);
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
    *c = (struct Pipeline) {NULL, 0, 0};

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

        *amp_occ-- = '\0';
        c->bg = 1;
        c->init_bg = 1;
    }
    else amp_occ = buffer + strlen(buffer)-1;

    // truncate dangling spaces/newlines at the end
    if (amp_occ != NULL) {
        while (*amp_occ == ' ' || *amp_occ == '\n')
            *amp_occ-- = '\0';
    }

    strncpy(c->usr_input, buffer, BUFFER_SIZE);

    while ((token = strsep(&buffer, "|")) != NULL){
        if (strcmp(token, "") != 0){


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
    if ((args->size > 1 && strcmp(args->tokens[0],"fg")==0) || (args->size > 2 && atoi(args->tokens[1]) && strcmp(args->tokens[0],"fg")==0))
        return FG;
    if ((args->size > 1 && strcmp(args->tokens[0],"bg")==0) || (args->size > 2 && atoi(args->tokens[1]) && strcmp(args->tokens[0],"bg")==0))
        return BG;
    if ((args->size <= 3 && strcmp(args->tokens[0],"cd")==0))
        return CD;



    return NO_CMD;
}
int wsh_execute(struct Pipeline *c, struct Program *args){

    // validate program - check if it is either a user program or built-in command
    char path[BUFFER_SIZE];
    snprintf(path, BUFFER_SIZE, "/usr/bin/%s", args->tokens[0]);
    int cmd_mode = is_built_in_cmd(args);

    // if exit is called, leave the shell
    // trying to call the fc ptr in the child will merely exit the child process
    if (cmd_mode == EXIT) built_in_cmds[EXIT](c, args);
    else if (cmd_mode > 1) { // change to 2 later
        if (c->size > 1){
            SAFE_EXIT("Misplaced built-in command\n");}
        else
        {built_in_cmds[cmd_mode](c, args);}
        return 0;
    }

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


    // pgid of any program chain is the lead (first child's) pid

    // if this program is the first in a chain, set group id to its own pid
    if (args==c->progs) {
        pgid = pid;
        c->pgid = pid;
    }

    // else, the first (lead) of the group is already initialized
    // set any successive programs in the chain to this pgid
    else pgid = c->pgid;


    // set group id of process
    // record group id in struct
    setpgid(pid, pgid);
    return 1;


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

void cmd_pipeline(struct Pipeline *cmd) {

    // load any file descriptors necessary to run the command pipeline
    int n_descriptors = ((cmd->size - 1) * 2) + 2;
    int pipes[n_descriptors];
    pipes[0] = STDIN_FILENO;
    pipes[n_descriptors - 1] = STDOUT_FILENO;
    for (int i = 1; i < n_descriptors - 1; i += 2) {

        int fd[2];
        if (pipe(fd) < 0) WSH_FAULT("failed to create pipe\n", -1);

        pipes[i] = fd[1]; // write end
        pipes[i + 1] = fd[0]; // read end

    }

    int rc;

    // exec progs
    for (int i = 0; i < cmd->size; i++) {

        cmd->progs[i].fd = pipes + (i * 2);
        cmd->progs[i].pip = pipes;
        cmd->progs[i].pip_size = n_descriptors;

        rc = wsh_execute(cmd, cmd->progs + i);
    }


    // close all file descriptors for clean-up
    close_fds(pipes, n_descriptors);


    // used to ensure waits are not issued on built_ins while background processes in op (BUG REPAIR)
    if (rc) {
        // wait on all children in the foreground

        if (!cmd->bg) {
            add_job(cmd, foreground, 0);
            for (int i = 0; i < cmd->size; i++) {
                int status; // change later, check output
                //fprintf(stderr, "VALUE: %p\n", (void*) (cmd->progs+i));
                waitpid(cmd->progs[i].pid, &status, WUNTRACED); // WUNTRACED, WNOHANG, use pid of each child!
            }
        } else add_job(cmd, background, 0);
/*
            printf("FIRST FOREGROUND JOBS\n");
            for (int i = 0; i < 10; i++){
                printf("%d: (%s,%d) ", i, foreground[i].usr_input, foreground[i].valid);
            }
            printf("\n");

            printf("FIRST BACKGROUND JOBS\n");
            for (int i = 0; i < 10; i++){
                printf("%d: (%s,%d) ", i, background[i].usr_input, background[i].valid);
            }
            printf("\n");
            */
        }


}



void sigint_handler(){
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (foreground[i].valid) {
            int status;
            waitpid(foreground[i].cmd->pgid, &status, WNOHANG);
            if (!WIFSTOPPED(status)) {
                killpg(foreground[i].cmd->pgid, SIGINT);
                foreground[i].valid = 0;
            }
        }
    }
}
void sigtstp_handler(){
   // printf("TRIGGERED\n");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (foreground[i].valid) {
           // printf("READY TO STOP: %s\n", foreground[i].usr_input);
            killpg(foreground[i].cmd->pgid, SIGSTOP);
            foreground[i].valid = 0;
            foreground[i].cmd->bg = 1;
            add_job(foreground[i].cmd, background,1);

        }
    }
}

void job_status(struct Job *job){
    if (job->valid){
       // printf("NEW JOB\n");
        int remove = 1;
        for (int i = 0; i < job->cmd->size; i++){
            int status;
            pid_t rc = waitpid(job->cmd->progs[i].pid, &status, WNOHANG); //pid_t rc =
            //printf("EVALUATING STATUS (rc %d, stopped %d, stat %d) OF: (%s,%d) with PID: %d\n", rc, WIFSTOPPED(status), WIFEXITED(status), job->cmd->usr_input, job->valid,job->cmd->progs[i].pid);
            //
            if (!WIFSIGNALED(status) || (!WIFEXITED(status) && rc >= 0)) remove = 0;

        }

        if (remove){

            // NOTE: move clean-up to add_jobs when a new job will be overwritten, and upon exit
           // printf("JOB TERMINATED: (%s,%d)\n", job->usr_input, job->valid);
            job->valid = 0;

        }

    }


}

void sigchld_handler(){

    // RACE CONDITION ON JOB STRUCTS, ACCESSING FREED DATA
    // NEED to add extra checks when
    for (int i = 0; i < BUFFER_SIZE; i++) {
        job_status(background+i);
        job_status(foreground+i);
    }


}

void configure_handlers(){
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        WSH_FAULT("Failed to bind sigint handler\n", -1);
    if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR)
        WSH_FAULT("Failed to bind sigint handler\n", -1);
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
        WSH_FAULT("Failed to bind sigint handler\n", -1);
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) // CHANGE BLOCKER LATER
        WSH_FAULT("Failed to bind sigint handler\n", -1);
    setpgid(0,0);
    tcsetpgrp(0, getpgrp());

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
    configure_handlers();

    while (1){

        // EOF CASE - CTRL + D, check this before printing shell prompt
        // so duplicate prompt doesn't print on force quit
        if (feof(stdin)) {
            if (!mode) {
                fclose(fp); // Free allocation to batch file if applicable
                SAFE_EXIT("") // buffer flushed by last exec call
            }
            SAFE_EXIT("\n");
        }

        if (mode) printf("wsh> "); // check if interactive mode (1)

        if (getline(&buffer, &BUFFER_SIZE, stdin) >= 0) {
            // if stdin stream is non-empty, process any inputs
            if (strlen(buffer) > 1) {

                // load all program calls/pipes by the user into command-chain struct
                // ENORMOUS BUG: things are getting set to stack pointer that gets destroyed! MALLOC THE CMD
                struct Pipeline *c = process_command(buffer);

                // execute command chain from user input
                cmd_pipeline(c);


            }
        }
    }

}
