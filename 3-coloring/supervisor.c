/**
 * @file supervisor.c
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 01.06.2019
 *
 * @brief Supervisor for 3 coloring
 **/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <semaphore.h>
#include "common.h"

static char *name = NULL;

/**
 * @brief Reads in all arguments and parses them.
 * @details Assures that no flags and arguments have been supplied. 
 * @param argc Number of arguments..
 * @param argv Argument Vector.
 * @return void
 */
static void getArgs(int argc, char **argv)
{
    name = argv[0];
    int opt;
    while ((opt = getopt(argc, argv, "")) != -1)
    {
        switch (opt)
        {
        default: /* '?' */
            fprintf(stderr, "Usage: %s\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc - optind != 0)
    {
        fprintf(stderr, "Usage: %s\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    getArgs(argc, argv);
    struct myshm *shared;

    int shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, PERMISSION);
    if (shmfd == -1)
        exitWithError(name, "shm open error");

    if (ftruncate(shmfd, sizeof(struct myshm)) == -1)
        exitWithError(name, "shm truncate error");

    shared = mmap(NULL, sizeof(struct myshm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shared == MAP_FAILED)
        exitWithError(name, "shm mmap error");

    shared->iWrite = 0; // in
    shared->iRead = 0;  // out
    shared->state = 1;  // run generators

    // init ring buffer
    for (int i = 0; i < MAX_SOLUTIONS; i++)
    {
        shared->rb.solutions[i].edges[0].from = i;
        shared->rb.solutions[i].edges[0].to = i;
    }

    sem_t *sem_write = sem_open(SEM_WRITE, O_CREAT | O_EXCL, 0600, MAX_SOLUTIONS); // consistency
    sem_t *sem_read = sem_open(SEM_READ, O_CREAT | O_EXCL, 0600, 0);               // consistency
    sem_t *sem_perm = sem_open(SEM_PERM, O_CREAT | O_EXCL, 0600, 0);               // mutex

    if (sem_write == SEM_FAILED || sem_read == SEM_FAILED || sem_perm == SEM_FAILED)
        exitWithError(name, "Could not open sem");

    // Handle Ringbuffer

    /* unmap shared memory */
    if (munmap(shared, sizeof *shared) == -1)
    {
        fprintf(stderr, "munmap");
        return -1;
    }

    // close(shm)
    if (close(shmfd) == -1)
    {
        fprintf(stderr, "close");
        return -1;
    }

    // shm_unlink()
    /* remove shared memory object */
    if (shm_unlink(SHM_NAME) == -1)
        exitWithError(name, "shm_unlink\n");

    // sem_close()
    sem_close(sem_write);
    sem_close(sem_read);
    sem_close(sem_perm);

    // sem_unlink()
    sem_unlink(SEM_WRITE);
    sem_unlink(SEM_READ);
    sem_unlink(SEM_PERM);

    exit(EXIT_SUCCESS);
}
