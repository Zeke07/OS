/*
 *
 *
 */

#include "wsh.h"



int wsh_exit(struct Args *args){
    exit(0);
}

int built_in_cmd(char **tokens, int num_tokens){

    if (strncmp(tokens[0],"exit", 4)==0){
        if (num_tokens <= 2 && (strcmp(tokens[0], "exit\n")==0 || ((strcmp(tokens[0], "exit")==0 && strcmp(tokens[1], "\n")==0))))
            return EXIT;
    }

    return NO_CMD;
}



struct Args process_inputs(char *buffer){

    char **tokens = NULL;
    char *token = NULL;
    int tok_size = 0;

    while((token = strsep(&buffer, " ")) != NULL){

        if (strcmp(token, "") != 0) {
            tokens = realloc(tokens, sizeof(char *) * (tok_size + 1));
            tokens[tok_size] = malloc(sizeof(char) * strlen(token));
            strcpy(tokens[tok_size], token);
            tok_size++;
        }
    }

    struct Args t = {tokens, tok_size};
    return t;

}


int main(int argc, char **argv){

    // interactive mode
    if (argc == 1){

        char *buffer;
        printf("wsh>");
        while (getline(&buffer, &BUFFER_SIZE, stdin) >= 0){

            // if stdin stream is non-empty, process any inputs
            if (strlen(buffer) > 0){

                struct Args tok = process_inputs(buffer);

                int cmd = built_in_cmd(tok.tokens, tok.size);
                if (cmd != 0)
                    built_in_cmds[cmd](&tok);



                for (int i = 0; i < tok.size; i++)
                    free(tok.tokens[i]);
                free(tok.tokens);

                printf("wsh>");

            }

        }

    }

    // batch input (shell script)
    else {

    }
    return 0;
}

