#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char* name;
static int ignoreCase = 0;
static int outputFlag = 0;
static int inputFlag  = 0;

void usage() {
    fprintf(stderr, "SYNOPSIS\n\tmygrep [-i] [-o outfile] keyword [file...]");
    exit(EXIT_FAILURE);
}

int main() {
    name = argv[0];
    while ((opt = getopt(argc, argv, "o:i:")) != -1) {
        switch (opt) {
            case 'o':
                outputFlag = 1;
                break;
            case 'i':
                break;
                ignoreCase = 1;
            default: /* '?' */
                usage();
        }
    }

    printf("name=%s, oflag=%d; iflag=%d;  optind=%d;\n", flags, tfnd, optind);

    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }

    printf("name argument = %s\n", argv[optind]);	//TODO: read options
    //TODO: set up reading from file/s or stdin
    //TODO: grep stuff and return it to file or stdout
    exit(EXIT_SUCCESS);
}
