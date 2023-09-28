/**
 * Zayn Khan
 * zkhan, zakhan5@wisc.edu
 *
 * wapropos cmd -> Lists any files and respective name descriptors if keyword is found
 * in Name or Description sections of manual pages
 *
 * Usage: ./wapropos <keyword>
 *
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/**
 * check for validity of provided file name to ensure it is a man page
 * must be of format <filename>.<digit>
 *
 * @param fn the filename to check
 * @return 1 if file name is valid, 0 otherwise
 */
int valid_filename(char fn[]){

    // check for occurrence of a '.', ensure its left neighbor is a letter, and the right is a number
    char *pos = strstr(fn, ".");
    if (pos == NULL || !isalpha(*(pos-1)) || !isdigit(*(pos+1))) return 0;

    return 1;

}

/**
 * indicate every manual page for the occurrence of <keyword> argument
 * within Name and Description sections
 *
 * @param argc num args
 * @param argv space delimited string args --> should be <keyword>
 * @return code 0 upon success, 1 upon fail
 */
int main(int argc, char** argv){

    // track if keyword is found in at least one file
    int exists_man = 0;

    // no arg
    if (argc == 1)
        printf("wapropos what?\n");

    // at least one arg, take the first one (argv[1])
    else {

        // init counter, buffer, directory pointer
        int i = 1;
        char buffer[256];
        DIR *dir;
        struct dirent *ent;


        // iterate over each sub-directory of man_pages man[1-9]
        while (i < 10) {

            snprintf(buffer, 256, "man_pages/man%d/", i);

            // open directory
            dir = opendir(buffer);
            if (dir != NULL) {

                // iterate over each entry/file of a single directory
                while ((ent = readdir(dir)) != NULL) {

                    // check if file entry is a valid man page, else skip
                    if (valid_filename(ent->d_name)) {


                        char filepath[512];
                        snprintf(filepath, 512, "%s%s", buffer, ent->d_name);

                        FILE *fp = fopen(filepath, "r");

                        // open file and search contents for keyword (argv[1])
                        if (fp != NULL) {

                            char line[256];

                            // buffer for name section contents in case keyword is a hit
                            char name_return[256];

                            // status bit for scanning over description section
                            int scanning_description = 0;

                            // track if keyword is found at least once in the file
                            int found = 0;


                            // iterate over file, find occurrence of keyword in name or description section
                            while (fgets(line, 256, fp)) {
                                if (strncmp(line, "\e[1mNAME\e[0m", 12) == 0) {
                                    fgets(name_return, 256, fp);

                                    if (strstr(name_return, argv[1]) != NULL) {
                                        found = 1;
                                        break;
                                    }

                                }
                                if (strncmp(line, "\e[1mDESCRIPTION\e[0m", 19) == 0) scanning_description = 1;

                                else if (strncmp(line, "\e[1m", 4) == 0) scanning_description = 0;

                                if (scanning_description && strstr(line, argv[1]) != NULL) {
                                    exists_man = 1;
                                    found = 1;
                                    break;
                                }
                            }

                            // if keyword is found, output the file name desc.
                            if (found) {
                                char *filename = strstr(ent->d_name, ".");

                                // truncate rhs of filename
                                *filename = '\0';
                                char *desc = strstr(name_return, "-");


                                printf("%s (%d) - %s", ent->d_name, i, desc + 2);

                            }

                            fclose(fp);


                        }

                        // failed to open valid file - program mal-function
                        else {
                            printf("File open failed\n");
                            exit(1);
                        }

                    }


                }

            }

            // failed to open valid directory (mal-function)
            else {
                printf("Directory open failed\n");
                exit(1);
            }

            closedir(dir);
            i++;

        }

        // keyword was never found
        if (!exists_man) printf("nothing appropriate\n");
    }

    return 0;

}

