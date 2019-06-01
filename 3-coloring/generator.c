#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h> /* For O_* constants */
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include "common.h"
#include <unistd.h>

static char *name = NULL;

void usage(void)
{
    printf("SYNOPSIS\n\tgenerator EDGE1...\nEXAMPLE\n\tgenerator 0-1 1-2 1-3 1-4 2-4 3-6 4-3 4-5 6-0\n");
    exit(EXIT_FAILURE);
}

int getEdge(char *arg, Edge *edge)
{
    // todo, handle errors
    char delim[] = "-";
    char *from = strsep(&arg, delim);
    edge->from = strtol(from, NULL, 10);
    edge->to = strtol(arg, NULL, 10);
    return 0;
}

int main(int argc, char *argv[])
{
    srand(time(NULL) ^ getpid());
    name = argv[0];

    // handle arguments
    int c = 0;
    while ((c = getopt(argc, argv, "")) != -1)
    {
        switch (c)
        {
            break;
        default:
        case '?':
            usage();
            break;
        }
    }

    if (argc - optind < 1)
        usage();

    int edgesCnt = argc - optind;
    Edge edges[edgesCnt];

    // read argument into program
    int knotsMax = 0;
    for (int i = 0; optind < argc; optind++)
    {
        char *h = strdup(argv[optind]);
        if (getEdge(h, &edges[i]) == -1)
            usage(); // free h

        if (edges[i].to > knotsMax)
            knotsMax = edges[i].to;
        if (edges[i].from > knotsMax)
            knotsMax = edges[i].from;
        i++;
        free(h);
    }

    for (int i = 0; i < edgesCnt; i++)
        printf("%d-%d ", edges[i].from, edges[i].to);

    printf("\nknotsMax: %d\n", knotsMax);

    // prepare vertexes
    Vertex vertex[knotsMax + 1];
    for (int i = 0; i <= knotsMax; i++)
        vertex[i].id = i;

    exit(EXIT_SUCCESS);
}