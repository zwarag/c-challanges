/**
 * @file common.h
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 01.06.2019
 *
 * @brief Shared Constants and Functions!
 **/

#include <stdio.h>
#include <stdlib.h>
#define SHM_NAME "/01529309_shm"
#define MAX_DATA (3072)
#define PERMISSION (0600)
#define MAX_EDGES 8
#define MAX_SOLUTIONS 32
#define SEM_WRITE "01529309_sem_write"
#define SEM_READ "01529309_sem_read"
#define SEM_PERM "01529309_sem_perm"

typedef struct edge
{
    int from;
    int to;
} Edge;

typedef struct solution
{
    Edge edges[MAX_EDGES];
} Solution;

typedef struct ringBuf
{
    Solution solutions[MAX_SOLUTIONS];
} RingBuf;

struct myshm
{
    unsigned int state, iWrite, iRead;
    RingBuf rb;
};

void exitWithError(char *name, char *message)
{
    fprintf(stderr, "%s: %s\n", name, message);
    exit(EXIT_FAILURE);
}
