/**
 * WSH Shell - Zayn Khan
 * Unix shell implementation with program execution,
 * pipe support, job management, foreground, background, signal-handling
 * support, and built-in commands
 *
 * NOTE: function descriptions are contained within header
 *
 * zkhan
 * zakhan5@wisc.edu
 * 9083145236
 *
 * Files: wsh.c, wsh.h
 */

#include "wsh.h"


struct Job jobs[256];
void wsh_fault(char *msg, int error_code){

    // free any allocated pipeline structures within the job structure upon termination
    for (int i = 0; i < BUFFER_SIZE; i++)
        if (jobs[i].cmd != NULL) wsh_clean(jobs[i].cmd);

    // exit with the indicated code, print usage message to stderr
    fprintf(stderr, "%s", msg);
    exit(error_code);
}

// simply exit
void wsh_exit(struct Pipeline *cmd, struct Program *args){
    if (args->size > 2) wsh_fault("Usage, too many args: exit\n", -1);
    wsh_fault("", 0); // buffer flushed by <enter> on exit call
}

void wsh_jobs(){

    for (int i = 0; i < BUFFER_SIZE; i++){

        // check job status
        job_status(jobs+i);

        // list if job is a valid background job
        if (jobs[i].valid && jobs[i].cmd->bg) {
            char line[BUFFER_SIZE];
            format_job(line, jobs+i, i+1);
            printf("%s", line);
        }
    }

}


void wsh_fg(struct Pipeline *cmd, struct Program *args) {

    // fetch id of job
    int id;
    if (args->size < 3) id = fetch_id();
    else id = atoi(args->tokens[1]) - 1;

    // if job valid, resume program if stopped, bring to foreground
    if (id >= 0 && id < BUFFER_SIZE){

        killpg(jobs[id].cmd->pgid, SIGCONT); // continue job
        jobs[id].cmd->bg = 0; // set background bit
        tcsetpgrp(0, jobs[id].cmd->pgid); // job now has terminal control

        // parent will wait on foreground job
        for (int i = 0; i < jobs[id].cmd->size; i++) {
            int status;
            waitpid(jobs[id].cmd->progs[i].pid, &status, WUNTRACED);

            // if ever suspended, change status to background
            if (!WIFEXITED(status)) jobs[id].cmd->bg = 1;
        }

        // if job is in the background, resume terminal control of the shell
        if (jobs[id].cmd->bg)
            tcsetpgrp(0, getpgid(getpid()));

        // otherwise, job has gracefully exited, clean resources, evict job slot
        else {
            jobs[id].valid = 0;
            wsh_clean(jobs[id].cmd);
            jobs[id].cmd = NULL;
        }
    }
    else
        wsh_fault("Bad ID specified: fg\n", -1);

}

void wsh_bg(struct Pipeline *cmd, struct Program *args){

    // fetch id of job
    int id;
    if (args->size < 3) id = fetch_id();
    else id = atoi(args->tokens[1]) - 1;

    // if valid id, continue resume job in background if suspended
    if (id >= 0 && id < BUFFER_SIZE)
        killpg(jobs[id].cmd->pgid, SIGCONT);
    else
        wsh_fault("Bad ID specified: bg\n", -1);

}


void wsh_cd(struct Pipeline *cmd, struct Program *args){

    // if #arg != 1, signal error, else execute chdir
    if (args->size == 3){
        int status = chdir(args->tokens[1]);
        if (status < 0) wsh_fault("Bad filepath: cd\n", -1);
    }
    else
        wsh_fault("Poor usage: cd <filepath>\n", -1);
}

void format_job(char string_builder[256], struct Job *job, int id){

    // format job listing as:
    // <id> : <command chain> [&]
    char *ptr = string_builder;
    snprintf(string_builder, BUFFER_SIZE, "%d: ", id);
    ptr+=3;
    snprintf(ptr, BUFFER_SIZE, "%s", job->cmd->usr_input);

}

int fetch_id(){

    // fetch the latest valid background job
    for (int i = BUFFER_SIZE-1; i >= 0; i--) {
        if (jobs[i].valid && jobs[i].cmd->bg)
            return i;
    }
    return -1;
}

void add_job(struct Pipeline *cmd){

    // insert job to the first invalid slot in the array
    int status = 0;
    for (int i = 0; i < BUFFER_SIZE; i++){

        // check if this slot is a stale process
        job_status(jobs+i);

        // if the slot is invalid, assign it
        if (!jobs[i].valid) {

            jobs[i].cmd = cmd;
            jobs[i].valid = 1;
            status = 1;
            break;
        }
    }

    // job list is full
    if (!status) wsh_fault("Job could not be appended\n", -1);
}

void job_status(struct Job *job){

    if (job->valid){

        // ensure that every process in a pipeline has terminated, only then can we evict it
        int remove = 1;
        for (int i = 0; i < job->cmd->size; i++){
            int status;

            // fetch an immediate status of the indicated process
            pid_t rc = waitpid(job->cmd->progs[i].pid, &status, WNOHANG);

            // if a process has terminated by way of signal or exited/returned in main
            // the job is not completed yet
            if (!WIFSIGNALED(status) || (!WIFEXITED(status) && rc >= 0))
                remove = 0;
        }

        // evict and clean struct resources if process is stale
        if (remove){
            job->valid = 0;
            wsh_clean(job->cmd);
            job->cmd = NULL;
        }


    }
}

struct Pipeline* process_command(char *buffer){

    // malloc a Pipeline, init fields
    struct Pipeline *c = malloc(sizeof(struct Pipeline));
    if (c == NULL)
        wsh_fault("Null pointer: malloc\n", -1);

    *c = (struct Pipeline) {NULL, 0, 0};

    // copy user input (used for printing jobs, assuming valid input)
    strncpy(c->usr_input, buffer, BUFFER_SIZE);

    // check for presence of a single & to check for background request
    char *token = NULL;
    char *amp_occ;
    if ((amp_occ = strstr(buffer, "&")) != NULL){
        if (strstr(amp_occ+1, "&") != NULL) {
            wsh_fault("Misplaced '&' token\n", -1);
        }
        else {
            char *i;
            i = amp_occ + 1;
            while (*i != '\0'){
                if (*i != EOF && *i != ' ' && *i != '\n')
                    wsh_fault("Misplaced '&' token\n", -1);
                i++;
            }
        }
        *amp_occ-- = '\0';
        c->bg = 1;

    }
    else amp_occ = buffer + strlen(buffer)-1;

    // truncate dangling spaces/newlines at the end
    if (amp_occ != NULL) {
        while (*amp_occ == ' ' || *amp_occ == '\n')
            *amp_occ-- = '\0';
    }

    // generate tokens by '|' operator, these represent each program
    while ((token = strsep(&buffer, "|")) != NULL){
        if (strcmp(token, "") != 0){

            // resize based on every subsequent program (token) that is generated
            c->progs = realloc(c->progs, sizeof(struct Program) * (c->size + 1));

            // each program (token) should be loaded into a Program struct and appended
            c->progs[c->size++] = process_inputs(token);
        }
    }

    // return the pipeline structure with all the program arguments loaded
    return c;
}


struct Program process_inputs(char *buffer){

    char **tokens = NULL; // array of string args to be stored
    char *token = NULL; // token buffer
    int tok_size = 0; // number of tokens (in this case, cmd arguments)

    while((token = strsep(&buffer, " ")) != NULL){

        if (strcmp(token, "") != 0 && strcmp(token, "\n") != 0) { // ignore empty and newline tokens

            // dynamically allocate memory based on argument sizes, resize accordingly
            tokens = realloc(tokens, sizeof(char *) * (tok_size + 1));
            tokens[tok_size] = malloc(sizeof(char) * strlen(token));
            strcpy(tokens[tok_size], token);
            tok_size++;
        }
    }

    // user program case: allocate null-pointer terminator for execvp arguments, is counted in size variable
    tokens = realloc(tokens, sizeof(char*) * (tok_size+1));
    tokens[tok_size++] = NULL;

    struct Program t = {tokens, tok_size};
    return t;

}

int is_built_in_cmd(struct Program *args){

    if (args->size >= 2 && strcmp(args->tokens[0],"exit")==0)
            return EXIT;
    if (args->size == 2 && strcmp(args->tokens[0],"jobs")==0)
        return JOBS;
    if ((args->size > 1 && strcmp(args->tokens[0],"fg")==0) || (args->size > 2 && atoi(args->tokens[1]) && strcmp(args->tokens[0],"fg")==0))
        return FG;
    if ((args->size > 1 && strcmp(args->tokens[0],"bg")==0) || (args->size > 2 && atoi(args->tokens[1]) && strcmp(args->tokens[0],"bg")==0))
        return BG;
    if ((args->size <= 3 && strcmp(args->tokens[0],"cd")==0))
        return CD;

    // code for no built-in
    return NO_CMD;
}

int wsh_execute(struct Pipeline *c, struct Program *args){

    // validate program - check if it is either a user program or built-in command
    char path[BUFFER_SIZE];
    snprintf(path, BUFFER_SIZE, "/usr/bin/%s", args->tokens[0]);
    int cmd_mode = is_built_in_cmd(args);

    // if exit is called, leave the shell
    // trying to call the fcn ptr in the child will merely exit the child process
    if (cmd_mode == EXIT) built_in_cmds[EXIT](c, args);

    // avoid spawning a child on built-ins
    // simply execute the function within the parent
    else if (cmd_mode > 0) {
        if (c->size > 1) wsh_fault("Misplaced built-in command\n", -1);
        else built_in_cmds[cmd_mode](c, args);
        return 0;
    }

    // spawn a new child process for executing user program
    pid_t pid = fork();

    // failed to spawn
    if (pid < 0) {
        wsh_fault("Failed to spawn process on fork\n", -1);
    } else if (pid == 0) { // CHILD

        // configure file-descriptors with stdin/out of the child
        if (args->fd[0] != STDIN_FILENO)
            dup2(args->fd[0], STDIN_FILENO);

        if (args->fd[1] != STDOUT_FILENO)
            dup2(args->fd[1], STDOUT_FILENO);

        // close any of the deep-copied file descriptors
        // to avoid blocks
        close_fds(args->pip, args->pip_size);

        // finally, execute the program
        execvp(args->tokens[0], args->tokens);

        // if execute did not exit() as intended, it failed
        wsh_fault("Failed to execute program: exec\n", -1);
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
        for (int j = 0; j < c->progs[i].size - 1; j++) // avoid NULL-ptr padding
            free(c->progs[i].tokens[j]);
        free(c->progs[i].tokens);
    }
    free(c->progs); // free process structs on heap
    free(c); // free pipeline struct
}

void close_fds(int *fds, int size){

    // close any open fds that are not stdin/stdout
    for (int i = 0; i < size; i++){
        if (fds[i] != STDIN_FILENO && fds[i] != STDOUT_FILENO)
            close(fds[i]);
    }
}

void cmd_pipeline(struct Pipeline *cmd) {

    // generate any file descriptors necessary to run the command pipeline
    int n_descriptors = (cmd->size * 2);
    int pipes[n_descriptors];
    pipes[0] = STDIN_FILENO;
    pipes[n_descriptors - 1] = STDOUT_FILENO;
    for (int i = 1; i < n_descriptors - 1; i += 2) {

        int fd[2];
        if (pipe(fd) < 0) wsh_fault("Failed to create pipe object\n", -1);

        pipes[i] = fd[1]; // write end
        pipes[i + 1] = fd[0]; // read end

    }

    int rc;

    // Execute programs in the order they are presented
    // designate the file descriptors on each iteration
    for (int i = 0; i < cmd->size; i++) {

        cmd->progs[i].fd = pipes + (i * 2);
        cmd->progs[i].pip = pipes;
        cmd->progs[i].pip_size = n_descriptors;

        rc = wsh_execute(cmd, cmd->progs + i);
    }


    // close all file descriptors for clean-up within the parent
    close_fds(pipes, n_descriptors);


    // job-processing and foreground management
    // for any user program pipelines
    if (rc) {

        // append current job to book-keeping structure
        add_job(cmd);

        // if job is initialized in the foreground
        // designate terminal control to the child, hold shell execution until
        // it is terminated/stopped, then revert terminal control to the shell
        if (!cmd->bg) {

            tcsetpgrp(0, cmd->pgid);
            for (int i = 0; i < cmd->size; i++) {
                int status;
                waitpid(cmd->progs[i].pid, &status, WUNTRACED);

                // if a change is observed, and the child has not yet exited,
                // it must have suspended
                if (!WIFEXITED(status)) cmd->bg = 1;
            }

            // revert TC to the parent
            tcsetpgrp(0, getpgid(getpid()));
        }
    }

}

void sigint_handler(){}
void sigtstp_handler(){}

void configure_handlers(){
    if (signal(SIGINT, sigint_handler) == SIG_ERR)
        wsh_fault("Failed to bind sigint handler\n", -1);
    if (signal(SIGTSTP, sigtstp_handler) == SIG_ERR)
        wsh_fault("Failed to bind sigint handler\n", -1);

    // block SIGTTOU for tcsetpgrp() functionality
    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR)
        wsh_fault("Failed to bind sigint handler\n", -1);

    // set the group id of the parent, so it is distinct from its parent
    // and ensure the shell has terminal control (especially if run using make)
    setpgid(0,0);
    tcsetpgrp(0, getpgid(getpid()));

}


/**
 * Wsh Shell Program -
 * 0 Args - run as an interactive shell
 * 1 Arg - execute commands within a file as a shell script
 *
 * @param argc num args
 * @param argv arg strings
 * @return
 */
int main(int argc, char **argv){
    int mode = INTERACTIVE; // default is interactive mode
    if (argc > 2) wsh_fault("Bad usage: ./wsh [batch-file]\n", -1);

    // swap to batch/script processing if execute has an arg
    // we do this by wrapping stdin with file arg in question
    // exit if the file cannot be read
    FILE *fp = NULL;
    if (argc == 2) {
        mode = BATCH;
        fp = freopen(argv[1], "r", stdin);
        if (fp == NULL) wsh_fault("Cannot read file: batch processing\n", -1);
    }

    // used to store user input to the shell
    char *buffer;

    // configure/block any handlers required to run the shell
    configure_handlers();

    while (1){

        // EOF CASE, batch process or CTRL+D termination
        if (feof(stdin)) {

            if (!mode) {
                fclose(fp); // Free allocation to batch file if applicable
                wsh_fault("", 0); // buffer flushed by last exec call
            }
            wsh_fault("\n", 0);
        }

        if (mode) printf("wsh> "); // print if interactive mode

        // process user input stream
        if (getline(&buffer, &BUFFER_SIZE, stdin) >= 0) {

            // if stdin stream is non-empty, process any inputs
            if (strlen(buffer) > 1) {

                // load all program calls/pipes by the user into command-chain struct
                struct Pipeline *c = process_command(buffer);

                // execute command chain from user's unpacked inputs
                cmd_pipeline(c);

            }
        }
    }
}
