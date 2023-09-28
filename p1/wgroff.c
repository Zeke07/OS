/**
 * Zayn Khan
 * zkhan, zakhan5@wisc.edu
 *
 * wgroff cmd -> convert formatted file to man page file format
 *
 *
 * Usage: ./wgroff <filepath>
 *
 */


#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// throw format error with specified line number of file
#define ERROR_CODE(line_num) {printf("Improper formatting on line %d\n", line_num); exit(0);}


// track tail line # of the file we are writing to
int count = 1;

int main(int argc, char** argv){

    // no arg, exit
    if (argc == 1){
        printf("Improper number of arguments\nUsage: ./wgroff <file>\n");
        exit(0);
    }


    // open file successfully if exists
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("File doesn't exist\n");
        exit(0);
    }

    // read first line
    char line[256];
    fgets(line, 256, fp);


    // traverse first line using space delimiter
    char *tok;
    tok = strtok(line, " ");


    // parse in stages, we must satisfy .TH, filename, section number and valid dat
    // if stage does not get incremented to 4 by the time the line ends, it is invalid
    int stages = 0;

    char filename[256];
    char date[256];
    char header[256];
    int section;
    char file[256];


    // parse first line by each space-separated word
    // first word must be .TH, then filename, etc
    while (stages != 4 && tok != NULL){

        switch (stages){
            case 0: // .TH
                if (strncmp(tok, ".TH", 3) != 0) {
                    fclose(fp);
                    ERROR_CODE(count);
                }

                stages++;
                break;

            case 1: // filename

                strcpy(file, tok);
                stages++;
                break;

            case 2: // section
                section = atoi(tok);
                if (section < 1 || section > 9) {
                    fclose(fp);
                    ERROR_CODE(count);
                }

                stages++;
                break;

            case 3: // verify date
                strcpy(date, tok);

                char date_tok[256];
                strcpy(date_tok, tok);

                tok = strtok(date_tok, "-");
                int date_stages = 0;


                // verify the date in stages, same as before
                // check length of each number of correct and within the correct number range
                while (date_stages != 3 && tok != NULL) {

                    switch (date_stages){
                        case 0: // YYYY
                            if (strlen(tok) != 4 || !atoi(tok)){
                                fclose(fp);
                                ERROR_CODE(count);
                            }
                            date_stages++;
                            break;
                        case 1: // MM
                            int month = atoi(tok);
                            if (strlen(tok) != 2 || month < 1 || month > 12) {
                                fclose(fp);
                                ERROR_CODE(count);
                            }

                            date_stages++;
                            break;
                        case 2: // DD --> assume DD\n
                            if (strlen(tok) != 3 && tok[2] != '\n') {
                                fclose(fp);
                                ERROR_CODE(count);
                            }

                            // strip newline
                            tok[2] = '\0';
                            int day = atoi(tok);
                            if (day < 1 || day > 31) {
                                fclose(fp);
                                ERROR_CODE(count);
                            }

                            date_stages++;
                            break;

                    }


                    tok = strtok(NULL, "-");
                }
                if (date_stages != 3) {
                    fclose(fp);
                    ERROR_CODE(count);
                }

                stages++;
                break;
        }

        tok = strtok(NULL, " ");
    }
    count++;


    size_t err;

    // generate filename for output file
    err = snprintf(filename, 256, "%s.%d", file, section);

    // should not enter if clause unless malfunction
    // checking the return of snprintf in case buffer is somehow too small
    if (err < 0) {
        printf("buffer overflow\n");
        exit(1);
    }

    // generate header in format filename(section#) for first line of out file
    err = snprintf(header, 256, "%s(%d)", file, section);
    if (err < 0) {
        printf("buffer overflow\n");
        exit(1);
    }

    // strip newline on date
    date[strlen(date)-1] = '\0';

    // left and right justify the header by adding padding
    // padding = ~80 chars - size of two header strs
    char justified[82];
    int padding = 80  - (2 * strlen(header));
    err = snprintf(justified, 82, "%s%*c%s\n", header, padding, ' ', header);

    // to avoid C complaining about writing "256 bytes" into buffer of 82.
    if (err < 0) {
        printf("buffer overflow\n");
        exit(1);
    }

    // begin writing to out file
    FILE *new_file = fopen(filename, "w");
    if (new_file == NULL){
        printf("Unable to open file\n");
        exit(1);
    }

    // output first line
    fprintf(new_file, "%s", justified);


    // read the rest of input file to output file using formatting restrictions
    while (fgets(line, 256, fp)){

        // read section .SH
        // capitalize section name, output with bold formatting to file
        if (strncmp(line, ".SH", 3) == 0){
            fprintf(new_file, "\n");

            // strip the spare newline in the word
            line[strlen(line)-1] = '\0';

            // capitalize everything after .SH in the line
            for (int i = 0; i < strlen(line+4); i++)
                line[i+4] = toupper(line[i+4]);

            fprintf(new_file, "\e[1m%s\e[0m\n", line+4);

        }

        // ignore comments
        else if (strncmp(line, "#", 1) == 0) continue;

        // read plain-text line (not a section header nor comment)
        else {


            int size = strlen(line);
            int i = 0;

            // indent with 7 spaces
            fprintf(new_file, "%*c", 7, ' ');

            // read each character of the line
            while (i < size){

                // check for //, replace with / in out file
                if (i + 1 < size && strncmp(line+i, "//", 2) == 0){
                    fprintf(new_file, "/");
                    i+=2;
                }

                // replace any other special characters
                else if (line[i] == '/' && i + 2 < size){

                    if (strncmp(line+i, "/fB", 3) == 0) fprintf(new_file, "\033[1m");
                    else if (strncmp(line+i, "/fI", 3) == 0) fprintf(new_file, "\033[3m");
                    else if (strncmp(line+i, "/fU", 3) == 0) fprintf(new_file, "\033[4m");
                    else if (strncmp(line+i, "/fP", 3) == 0) fprintf(new_file, "\033[0m");

                    i+=3;
                }

                // if not a special character or //, output to file as normal
                else {
                    fprintf(new_file, "%c", line[i]);
                    i++;
                }

            }
        }

        count++;

    }


    // output final line that has date, center it by padding spaces to lhs and rhs
    padding = (80 - strlen(date))/2;
    fprintf(new_file, "%*c%s%*c\n", padding, ' ', date, padding, ' ');



    count++;
    fclose(new_file);
    fclose(fp);

    return 0;
}