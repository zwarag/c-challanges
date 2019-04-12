/**
 * @file mygrep.c
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 12.04.2019
 * 
 * @brief filters STDIN or multiple Files for a keyword. 
 * 
 * This Program allows to filter a input for a keyword.
 **/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static char* name;
static int ignoreCase = 1;
static int inputFlag  = 0;
static int outputFlag = 0;
static char* keyword;
static char* inputFile;
static char* outputFile;

static int debug = 0;

/**
 * Mandatory usage function.
 * @brief This function writes helpful usage information about the program to stderr.
 * @returns void
 */
void usage() {
    fprintf(stderr, "SYNOPSIS\n\tmygrep [-i] [-o outfile] keyword [file...]\n");
    exit(EXIT_FAILURE);
}

/**
 * Main program function.
 * @brief Will read input (file or stdin) and filter lines that contain the searched keyword.
 * @param argc The argument counter.
 * @param argv A list of Arguments.
 * @returns void
 */
int main(int argc, char *argv[]) {

    // HANDLE ARGUMENTS
    int opt;
    name = argv[0];
    while ((opt = getopt(argc, argv, "o:i")) != -1) {
        switch (opt) {
            case 'o':
                outputFlag = 1;
                outputFile = optarg;
                if( !access( outputFile, F_OK|W_OK) != -1 ) {
                    FILE *fp = fopen(outputFile, "wb");
                    if (!fp) { 
                        fprintf(stderr, "%s: [ERROR] could not create or open file \"%s\" to write results into!\n", name, outputFile);
                        usage();
                    } 
                } 
                break;
            case 'i':
                ignoreCase = 0;
                break;
            default: /* '?' */
                usage();
        }
    }

    if(optind < argc) {
        keyword = argv[optind++];
    } else { // no keword 
        usage();
    }

    int optindOrig = optind;
    while (optind < argc) {
        inputFlag = 1;
        // Check if inputFiles are readable;
        inputFile = argv[optind++];
        if( access( inputFile, F_OK|R_OK ) != -1 ) {
        } else {
            fprintf(stderr, "%s: [ERROR] \"%s\" file does not exist or is not readble!\n", name, inputFile);
            usage();
        } 
    }
    optind = optindOrig;
    if(debug == 1) {
        printf("name=%s; keyword=%s, ignoreCase=%d; optdiff=%d;\noutputFlag=%d;\toutputFile=%s;\ninputFlag=%d;\tintputFile=%s;\n", name, keyword, ignoreCase, argc-optind, outputFlag, outputFile, inputFlag, inputFile);
    }

    // SETUP Read and Write ends.
    if(ignoreCase == 0) {
        for(int i = 0; keyword[i]; i++){
            keyword[i] = tolower(keyword[i]);
        }
    }
    FILE* fp_read;
    FILE* fp_write;
    if(!inputFlag) {
        fp_read = stdin;
    }
    if(!outputFlag) {
        fp_write = stdout;
    } else {
        fp_write = fopen(outputFile, "w");
        if(fp_write == NULL) {
            exit(EXIT_FAILURE);
        }
    }

    // MAIN GREP LOOP
    while(optind < argc || (inputFlag == 0) ) {
        char *line = NULL;
        char *origLine = NULL;
        size_t len = 0;
        ssize_t read;

        if(inputFlag) {
            fp_read = fopen(argv[optind++], "r");
            if (fp_read == NULL)
                exit(EXIT_FAILURE);
        }
        while ((read = getline(&line, &len, fp_read)) != -1) { // read from file/stdin
            if( (!inputFlag) && strlen(line) == 1 ) {
                inputFlag=1; // to exit while loop;
                break;
            }
            origLine = strdup(line);
            if(origLine == NULL) {
                exit(EXIT_FAILURE);
            }
            if(ignoreCase == 0) {
                for(int i = 0; line[i]; i++){
                    line[i] = tolower(line[i]);
                }
            }
            if(strstr(line, keyword)) {
                fprintf(fp_write, "%s", origLine); // write match to file/stdout
            }
        }
        free(origLine);
        free(line);
    }
    fclose(fp_write);
    fclose(fp_read);

    exit(EXIT_SUCCESS);
}
