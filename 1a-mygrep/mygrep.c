#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char* name;
static int ignoreCase = 1;
static int inputFlag  = 0;
static int outputFlag = 0;
static char* keyword;
static char* inputFile;
static char* outputFile;

void usage() {
    fprintf(stderr, "SYNOPSIS\n\tmygrep [-i] [-o outfile] keyword [file...]\n");
    exit(EXIT_FAILURE);
}

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
        printf(inputFile);
        printf("\n");
        if( access( inputFile, F_OK|R_OK ) != -1 ) {
        } else {
            fprintf(stderr, "%s: [ERROR] \"%s\" file does not exist or is not readble!\n", name, inputFile);
            usage();
        } 
    }
    optind = optindOrig;
    printf("name=%s; keyword=%s, ignoreCase=%d; optdiff=%d;\noutputFlag=%d;\toutputFile=%s;\ninputFlag=%d;\tintputFile=%s;\n", name, keyword, ignoreCase, argc-optind, outputFlag, outputFile, inputFlag, inputFile);

    //TODO: set up reading from file/s or stdin
    FILE* fp_read;
    FILE* fp_write;
    if(!inputFlag) {
       fp_read = stdin;
    }

    while(optind < argc || (inputFlag == 0) ) {
        char *line = NULL;
        size_t len = 0;
        ssize_t read;

        if(inputFlag) {
            fp_read = fopen(argv[optind++], "r");
            if (fp_read == NULL)
                exit(EXIT_FAILURE);
        }

        while ((read = getline(&line, &len, fp_read)) != -1) {
            printf("Retrieved line of length %zu :\n", read);
            printf("%s", line);
            if( (!inputFlag) && strlen(line) == 1 ) {
                inputFlag=1; // to exit while loop;
                break;
            }
            if(strstr(line, keyword)) {
                printf("%s: ^^^^^^ contains %s\n", name, keyword); // print stuff
            }
        }
        free(line);
    }

    //TODO: grep stuff and return it to file or stdout
    exit(EXIT_SUCCESS);
}
