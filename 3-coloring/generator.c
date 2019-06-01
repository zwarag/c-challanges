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

    struct myshm *shared;

    int shmfd = shm_open(SHM_NAME, O_RDWR, PERMISSION);
    if (shmfd == -1)
        exitWithError(name, "shm open error");

    shared = mmap(NULL, sizeof(struct myshm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shared == MAP_FAILED)
        exitWithError(name, "shm mmap error");

    // sem_open()
    sem_t *sem_write = sem_open(SEM_WRITE, 0);
    sem_t *sem_read = sem_open(SEM_READ, 0);
    sem_t *sem_perm = sem_open(SEM_PERM, 0);

    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED || sem_perm == SEM_FAILED)
        exitWithError(name, "Could not open sem");

    Solution solution;
    for (;;)
    {
        // init solution
        for (int i = 0; i < MAX_EDGES; i++)
        {
            solution.edges[i].to = 0;
            solution.edges[i].from = 0;
        }

        // random coloring
        for (int i = 0; i <= knotsMax; i++)
            vertex[i].col = rand() % 3;

        int h = 0;
        for (int i = 0; i < edgesCnt; i++)
        {
            if (vertex[edges[i].from].col == vertex[edges[i].to].col)
            {
                solution.edges[h].from = edges[i].from;
                solution.edges[h].to = edges[i].to;
                h++;
                if (h >= MAX_EDGES)
                {
                    h = -1;
                    break;
                }
            }
        }

        if (h >= 0)
        {

            if (sem_wait(sem_write) == -1)
            {
                if (errno == EINTR) // interrupted by signal?
                    continue;
                exitWithError(name, "other error"); // other error
            }
            if (sem_wait(sem_perm) == -1)
            {
                if (errno == EINTR) // interrupted by signal?
                    continue;
                exitWithError(name, "other error"); // other error
            }

            // check if supervisor wants to stop
            memcpy(&shared->rb.solutions[shared->iWrite], &solution, sizeof(Solution));
            if (shared->iWrite == MAX_SOLUTIONS - 1)
                shared->iWrite = 0;
            else
                shared->iWrite++;
            sem_post(sem_perm);
            sem_post(sem_read);
            if (h == 0)
                break;
        }
    }

    /* unmap shared memory */
    if (munmap(shared, sizeof *shared) == -1)
        exitWithError(name, "munmap");

    // close(shm)
    if (close(shmfd) == -1)
        exitWithError(name, "close");

    // sem_close()
    sem_close(sem_write);
    sem_close(sem_read);
    sem_close(sem_perm);

    exit(EXIT_SUCCESS);
}