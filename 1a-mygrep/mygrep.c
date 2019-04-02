#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char* name;
static int ignoreCase = 1;
static int inputFlag  = 0;
static int outputFlag = 0;
static char* inputFile;
static char* outputFile;

void usage() {
    fprintf(stderr, "SYNOPSIS\n\tmygrep [-i] [-o outfile] keyword [file...]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int opt;
    name = argv[0];
    while ((opt = getopt(argc, argv, "o:i")) != -1) {
        switch (opt) {
            case 'o':
                outputFlag = 1;
                outputFile = optarg;
                break;
            case 'i':
                ignoreCase = 0;
                break;
            default: /* '?' */
                usage();
        }
    }

    if (optind < argc) {
        //TODO: check that file exists
        inputFlag = 1;
        inputFile = argv[optind++];
    }

    printf("name=%s; ignoreCase=%d; optind=%d;\noutputFlag=%d;\toutputFile=%s;\ninputFlag=%d;\tintputFile=%s;\n", name, ignoreCase, optind, outputFlag, outputFile, inputFlag, inputFile);

    //TODO: set up reading from file/s or stdin
    //TODO: grep stuff and return it to file or stdout
    exit(EXIT_SUCCESS);
}
