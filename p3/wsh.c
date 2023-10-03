/*
 *
 *
 */

#include "wsh.h"



int wsh_exit(struct Prog *args){
    exit(EXIT_SUCCESS);
}


struct CMD process_command(char *buffer){
    struct CMD c = {NULL, 0};
    char *token = NULL;


    while ((token = strsep(&buffer, "|")) != NULL){
        if (strcmp(token, "") != 0){

            // truncate a newline if it exists
            size_t tok_len = strlen(token);
            if (token[tok_len-1] == '\n') {
                token[tok_len-1] = '\0';
                // add bit here if needed
            }
            c.progs = realloc(c.progs, sizeof(struct Prog) * (c.size + 1));
            c.progs[c.size++] = process_inputs(token);
        }
    }

    return c;
}

struct Prog process_inputs(char *buffer){

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

    struct Prog t = {tokens, tok_size};
    return t;

}

int is_built_in_cmd(struct Prog *args){

    if (args->size == 2 && strcmp(args->tokens[0],"exit")==0)
            return EXIT;


    return NO_CMD;
}
void wsh_execute(struct Prog *args){

    char path[256];
    snprintf(path, 256, "/usr/bin/%s", args->tokens[0]);
    int cmd_mode = is_built_in_cmd(args);
    if (cmd_mode != NO_CMD) built_in_cmds[cmd_mode](args);
    else {

        // execute user program
        pid_t pid = fork();

        if (pid < 0) {
            FAIL("Failed to spawn process on fork\n", -1);
        } else if (pid == 0) {


            if (args->fd[0] != STDIN_FILENO)
                dup2(args->fd[0], STDIN_FILENO);

            if (args->fd[1] != STDOUT_FILENO)
                dup2(args->fd[1], STDOUT_FILENO);


            close_fds(args->pip, args->pip_size);

            //for (int i = 0; i < args->pip_size; i++)
            //    printf("Status of %d: (%d)\n", args->pip[i], fcntl(args->pip[i], F_GETFD));

            // exec user prog
            execvp(args->tokens[0], args->tokens);
            FAIL("Failed to execute program", -1);
        }

    }

}
void wsh_clean(struct CMD *c){
    for (int i = 0; i < c->size; i++) {
        for (int j = 0; j < c->progs[i].size - 1; j++)
            free(c->progs[i].tokens[j]);
        free(c->progs[i].tokens);
    }
    free(c->progs);
}

void close_fds(int *fds, int size){
    for (int i = 0; i < size; i++){
        if (fds[i] != STDIN_FILENO && fds[i] != STDOUT_FILENO)
            close(fds[i]);
    }
}

void cmd_pipeline(struct CMD *cmd){

    // load any file descriptors necessary to run the command pipeline
    int n_descriptors = ((cmd->size-1)*2)+2;
    int pipes[n_descriptors];
    pipes[0] = STDIN_FILENO;
    pipes[n_descriptors-1] = STDOUT_FILENO;
    for (int i = 1; i < n_descriptors-1; i+=2){

        int fd[2];
        if (pipe(fd) < 0)
            FAIL("failed to create pipe", -1);

        pipes[i] = fd[1]; // write end of
        pipes[i+1] = fd[0]; // read end

    }

    // exec progs
    for (int i = 0; i < cmd->size; i++) {

        cmd->progs[i].fd = pipes+(i*2);
        cmd->progs[i].pip = pipes;
        cmd->progs[i].pip_size = n_descriptors;
        wsh_execute(cmd->progs+i);

    }

    // close all file descriptors for clean-up
    close_fds(pipes, n_descriptors);

    // wait on all children - CHANGE ONCE GPID IMPLEMENTED
    for (int i = 0; i < cmd->size; i++){
        int status; // change later
        waitpid(-1, &status, WUNTRACED);
    }


}

int main(int argc, char **argv){
    int mode = INTERACTIVE; // default is interactive mode

    // swap to batch processing if execute has an arg
    FILE *fp = NULL;
    if (argc > 1) {
        mode = BATCH;
        fp = freopen(argv[1], "r", stdin);
        if (fp == NULL) FAIL("Null file pointer\n", -1);
    }

    // NOTE: things to add to clean --> free any mallocs on exit, close any open files
    char *buffer; // <NULL>
    if (mode) printf("wsh> ");
    while (getline(&buffer, &BUFFER_SIZE, stdin) >= 0){

        // if stdin stream is non-empty, process any inputs
        if (strlen(buffer) > 0){

            // load all program calls/pipes by the user into command-chain struct
            struct CMD c = process_command(buffer);

            // execute command chain from user input
            cmd_pipeline(&c);

            // free allocated buffers, initiate next prompt
            wsh_clean(&c);
            if (mode) printf("wsh> ");

        }

    }

    fclose(fp); // Free allocation to batch file
    return 0;
}

