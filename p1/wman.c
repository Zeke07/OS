/**
 * Zayn Khan
 * zkhan, zakhan5@wisc.edu
 *
 * wman cmd -> output contents of specified manual page located in the man_pages directory
 *
 *
 * Usages:
 *  ./wman <section#> <filename>
 *  ./wman <filename>
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * print contents of file if found
 *
 * @param argc num arguments
 * @param argv space delimited arg strings either <filename> or <section> <filename>
 * @return code 0 upon success, 1 on fail
 */
int main(int argc, char** argv) {

    // case 1: filename provided only
    if (argc == 2){
        int i = 1;

        // loop over man[1-9] in man_pages dir
        while (i < 10)
        {
           char buffer[256];

           snprintf(buffer, 256, "man_pages/man%d/%s.%d", i, argv[1], i);

           // open file with path man_pages/man[1-9]/<filename>
           FILE *fp = fopen(buffer, "r");

           // if the file exists, print out the contents char-by-char
           if (fp != NULL) {
                char c = fgetc(fp);
                while (c != EOF)
                {
                    printf("%c", c);
                    c = fgetc(fp);
                }
                fclose(fp);

                return 0;
           }

           i += 1;
        }

        // specified file does not exist
        printf("No manual entry for %s\n", argv[1]);

    }

    // case 2: filename AND section provided
    else if (argc >= 3)
    {
        int section = atoi(argv[1]);

        // check if file format is correct and that section number is a valid int within range [1-9]
        if (strstr(argv[1], ".") != NULL || section < 1 || section > 9 || strlen(argv[1]) != 1)
        {
            printf("invalid section\n");
            exit(1);
        }

        char buffer[256];
        snprintf(buffer, 256, "man_pages/man%d/%s.%d", section, argv[2], section);
        FILE *fp = fopen(buffer, "r");


        // file does not exist in the specified section
        if (fp == NULL) printf("No manual entry for %s in section %d\n", argv[2], section);

        // else print contents if file exists
        else {
            char c = fgetc(fp);
            while (c != EOF)
            {
                printf("%c", c);
                c = fgetc(fp);
            }
            fclose(fp);
        }

    }

    // no argument specified
    else printf("What manual page do you want?\nFor example, try 'wman wman'\n");

    return 0;
}