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


    if (access(path, X_OK) != 0 && access(path+4, X_OK) != 0) printf("Command not found\n");

    else { // execute user program

        pid_t pid = fork();

        if (pid < 0) {
            FAIL("Failed to spawn process on fork\n", -1);
        } else if (pid == 0) {


            if (args->fd[0] != STDIN_FILENO) {
                dup2(args->fd[0], STDIN_FILENO);
                close(args->fd[0]);
            }
            if (args->fd[1] != STDOUT_FILENO) {
               dup2(args->fd[1], STDOUT_FILENO);
               close(args->fd[1]);
            }


            //printf("PAIR: (%d, %d)\n", args->fd[0], args->fd[1]);
            //dup2(args->fd[0], args->fd[1]);
            //close(args->fd[1]);

            execvp(args->tokens[0], args->tokens);
            FAIL("Failed to execute program", -1);
        }
        else {
            int status;
            wait(&status);
            if (status < 0)
                FAIL("Wait fail", -1);


        }
    }

}

void cmd_pipeline(struct CMD *cmd){

    int n_descriptors = ((cmd->size-1)*2)+2;
    int pipes[n_descriptors];
    pipes[0] = STDIN_FILENO;
    pipes[n_descriptors-1] = STDOUT_FILENO;
    //printf("Num Desc: %d\n", n_descriptors);
    for (int i = 1; i < n_descriptors-1; i+=2){

        int fd[2];
        if (pipe(fd) < 0)
            FAIL("failed to create pipe", -1);

        pipes[i] = fd[1]; // write end
        pipes[i+1] = fd[0]; // read end

    }

    // exec progs

    for (int i = 0; i < cmd->size; i++) {

        cmd->progs[i].fd = pipes+(i*2);
        printf("PAIR: (%d, %d)\n", cmd->progs[i].fd[0], cmd->progs[i].fd[1]);
        wsh_execute(cmd->progs+i);

    }

    // close any new file descriptors
    for (int i = 1; i < n_descriptors-1; i++) close(pipes[i]);

    /*
    cmd->progs[0].fd[0] = 1;
    cmd->progs[0].fd[1] = 4;
    cmd->progs[1].fd[0] = 0;
    cmd->progs[1].fd[1] = 3;

    wsh_execute(cmd->progs);
    wsh_execute(cmd->progs+1);


    int STD = 0;
    int nfd[2];
    for (int i = 0; i < cmd->size; i++){
        if (STD % 2 == 0 && pipe(nfd) < 0) FAIL("failed to create pipe", -1);
            cmd->progs[i].fd[0] = STD+1%2;
            cmd->progs[i].fd[1] = nfd[(STD+1)%2];
            STD++;
            STD %= 2;

        wsh_execute(cmd->progs+i);
    }

    // init file descriptors to link adjacent programs (there are n-1 pipes, n+1 descriptors)
    int pipes[cmd->size+1];
    //pipes[0] = STDIN_FILENO;
    pipes[cmd->size] = STDOUT_FILENO;

     for (int i = 0; i < cmd->size; i+=2){
         if (i != cmd->size-1) {
             if (pipe(pipes + i) < 0) FAIL("failed to create pipe", -1);
         }
         else {
             int fds[2];
             if (pipe(fds) < 0) FAIL("failed to create pipe", -1);
             pipes[i] = fds[0];
             close(fds[1]);
         }
     }


    for (int i = 0; i < cmd->size+1; i++)
        printf("Status of %d: (%d)\n", pipes[i], fcntl(pipes[i], F_GETFD));




    for (int i = 0; i < cmd->size; i++) {
        //if (pipe(cmd->progs[i].fd < 0)) FAIL("failed to create pipe", -1);
        cmd->progs[i].fd[0] = pipes[i];
        cmd->progs[i].fd[1] = pipes[i+1];
        wsh_execute(cmd->progs+i);

    }


     for (int i = 0; i < cmd->size; i++) {
         printf("Status of %d: (%d)\n", pipes[i], fcntl(pipes[i], F_GETFD));
         close(pipes[i]);
     }
     */

}

int main(int argc, char **argv){

    // interactive mode
    if (argc == 1){

        char *buffer;
        printf("wsh>");
        while (getline(&buffer, &BUFFER_SIZE, stdin) >= 0){

            // if stdin stream is non-empty, process any inputs
            if (strlen(buffer) > 0){
                    struct CMD c = process_command(buffer);

                    // execute command chain from user input
                    cmd_pipeline(&c);

                    /* DEBUG
                    for (int i = 0; i < c.size; i++){
                        for (int j = 0; j < c.progs[i].size-1; j++){
                            printf("%s", c.progs[i].tokens[j]);
                        }
                    }
                    */
                /* REFACTOR
                // this only runs a single program
                struct Prog tok = process_inputs(buffer);

                // built-in commands
                int cmd = is_built_in_cmd(&tok);
                if (cmd != 0)
                    built_in_cmds[cmd](&tok);

                // user program execute
                wsh_execute(&tok);

                }
                */

                // free allocated buffers, initiate next prompt
                for (int i = 0; i < c.size; i++) {
                    for (int j = 0; j < c.progs[i].size - 1; j++)
                        free(c.progs[i].tokens[j]);
                    free(c.progs[i].tokens);
                }
                free(c.progs);

                printf("wsh>");

            }

        }

    }

    // batch input (shell script)
    else {

    }
    return 0;
}

