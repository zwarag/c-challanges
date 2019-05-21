/**
 * @file client.c
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 11.11.2018
 * 
 * @brief HTTP Client Software
 * 
 * @detail: This Program allows to load Files from HTTP Server supporting HTTP/1.1.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define CHILD_NUM 4
#define BUF_SIZE 4

#define PIPE_R 0
#define PIPE_W 1
#define PIPE_TO_C 0
#define PIPE_FROM_C 1
static int pipes[CHILD_NUM][2][2];

static char *name = NULL;
static char *strNum1 = NULL;
static char *strNum2 = NULL;
static int inputLength = 0;
static char *Al = NULL;
static char *Ah = NULL;
static char *Bl = NULL;
static char *Bh = NULL;

void getArgs(int argc, char **argv)
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

char *readString()
{
    char buf[BUF_SIZE];
    char *str;
    size_t strSize = 1; // including the 0 after the following malloc
    str = malloc(sizeof(char) * BUF_SIZE);
    if (str == NULL)
    {
        fprintf(stderr, "%s, ERROR: could not allocate enough memory!\n", name);
        exit(EXIT_FAILURE);
    }
    str[0] = '\0';
    while (fgets(buf, BUF_SIZE, stdin))
    {
        char *from = str;
        strSize += strlen(buf);
        str = realloc(str, strSize);
        if (str == NULL)
        {
            fprintf(stderr, "%s, ERROR: could not rellocate enough memory!\n", name);
            free(from);
            exit(EXIT_FAILURE);
        }
        strcat(str, buf);
        char *pos;
        if ((pos = strchr(buf, '\n')) != NULL)
            break;
    }
    if (ferror(stdin))
    {
        fprintf(stderr, "%s, ERROR: could not read from stdin!\n", name);
        free(str);
        exit(EXIT_FAILURE);
    }
    return str;
}

unsigned int parseChar(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return 10 + c - 'a';
    if ('A' <= c && c <= 'F')
        return 10 + c - 'A';
    abort();
}

/**
 * @param void
 **/

void generatePointers(void)
{
    fprintf(stderr, "generating pointers\n");
    Al = strNum1;
    Ah = strNum1 + (inputLength / 2);
    Bl = strNum2;
    Bh = strNum2 + (inputLength / 2);
}

int main(int argc, char **argv)
{
    // Handle getopt
    getArgs(argc, argv);

    // Read into dynamic buffers
    strNum1 = readString();
    strNum2 = readString();

    //TODO: check equal length
    if (strlen(strNum1) != strlen(strNum2))
    {
        fprintf(stderr, "%s: inputs do not have equal length!\n", name);
        exit(EXIT_FAILURE);
    }
    inputLength = strlen(strNum1);

    if (strlen(strNum1) < 2 || strlen(strNum2) < 2)
    {
        fprintf(stderr, "%s: got not inputs!\n", name);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "A %ld: %sB %ld: %s", strlen(strNum1), strNum1, strlen(strNum2), strNum2);

    // check length is one and multiply
    if (strlen(strNum1) == 2 && strlen(strNum2) == 2)
    {
        fprintf(stdout, "%1x\n", parseChar(strNum1[0]) * parseChar(strNum2[0]));
        exit(EXIT_SUCCESS);
    }
    else
    {
        generatePointers();
    }

    //TODO: Fork work to childs
    pid_t procs[CHILD_NUM] = {-1, -1, -1, -1};
    //for (int i = 0; i < CHILD_NUM + 20; i++)
    //{
    //    fprintf(stderr, "%s: procs: %ld\n", name, procs[i]);
    //}
    int childs = 0;
    int status[CHILD_NUM];

    for (int i = 0; i < CHILD_NUM; i++)
    {
        if (pipe(pipes[i][PIPE_R]) == -1)
        {
            fprintf(stderr, "%s: pipe error!\n", name);
            exit(EXIT_FAILURE);
        }
        if (pipe(pipes[i][PIPE_W]) == -1)
        {
            fprintf(stderr, "%s: pipe error!\n", name);
            exit(EXIT_FAILURE);
        }
    }

    do
    {
        procs[childs] = fork();
        if (procs[childs] == -1)
        {
            fprintf(stderr, "%s: failed to fork!\n", name);
            exit(EXIT_FAILURE);
        }
        if (procs[childs] == 0)
            break;
        childs++;
    } while (childs < CHILD_NUM);
    if (childs == 4)
        childs--;

    if (procs[childs] == 0)
    { // child
        // close own pipes
        close(pipes[childs][PIPE_FROM_C][PIPE_R]);
        close(pipes[childs][PIPE_TO_C][PIPE_W]);

        // close pipes to other childs
        for (int i = 0; i < CHILD_NUM; i++)
        {
            if (i == childs)
            {
                continue;
            }
            else
            {
                close(pipes[i][PIPE_FROM_C][PIPE_R]);
                close(pipes[i][PIPE_FROM_C][PIPE_W]);
                close(pipes[i][PIPE_TO_C][PIPE_R]);
                close(pipes[i][PIPE_TO_C][PIPE_W]);
            }
        }

        // route pipes
        dup2(pipes[childs][PIPE_FROM_C][PIPE_W], STDOUT_FILENO);
        dup2(pipes[childs][PIPE_TO_C][PIPE_R], STDIN_FILENO);

        //execlp(name, name, NULL);
        char *num = NULL;
        size_t chars = 0;
        FILE *fd = fdopen(pipes[childs][PIPE_TO_C][PIPE_R], "r");
        if (getline(&num, &chars, fd) > 0)
            fprintf(stderr, "%d", childs + 10);
        fclose(fd);
        fprintf(stdout, "%d", childs + 20);
        fflush(stdout);
        fprintf(stderr, "%s: child %d done read %s.\n", name, childs, num);
        //fprintf(stderr, "%s: child %d done read.\n", name, childs);
        close(pipes[childs][PIPE_FROM_C][PIPE_W]);
        close(pipes[childs][PIPE_TO_C][PIPE_R]);
    }
    else
    { // parent

        fprintf(stderr, "%s: parent 1\n", name);
        // close pipes to other childs
        for (int i = 0; i < CHILD_NUM; i++)
        {
            close(pipes[i][PIPE_FROM_C][PIPE_W]);
            close(pipes[i][PIPE_TO_C][PIPE_R]);
        }

        // pass data down
        int h = strlen(strNum1) / CHILD_NUM;
        int run = 0;
        for (int i = 0; i < CHILD_NUM; i++)
        {
            dprintf(pipes[i][PIPE_TO_C][PIPE_W], "%d\n", i + 100);
        }
        for (int i = 0; i < CHILD_NUM / 2; i++)
        {
            //while (run < h * i + 1)
            //{
            //    //dprintf(pipes[i][PIPE_TO_C][PIPE_W],
            //    //"%1x", floatarr_get(&even, i));
            //}
            close(pipes[i][PIPE_TO_C][PIPE_W]);
        }
        for (int i = 2; i < CHILD_NUM; i++)
        {
            //dprintf(pipes[i][PIPE_TO_C][PIPE_W],
            //        "%f\n", floatarr_get(&even, i));
            close(pipes[i][PIPE_TO_C][PIPE_W]);
        }
        fflush(stdout);
        fprintf(stderr, "%s: parent 2\n", name);

        //read from pipes
        char *results[CHILD_NUM] = {NULL, NULL, NULL, NULL};
        size_t chars[CHILD_NUM];
        for (int i = 0; i < CHILD_NUM; i++)
        {
            FILE *fd = fdopen(pipes[i][PIPE_FROM_C][PIPE_R], "r");
            if (getline(&results[i], &chars[i], fd) == -1)
            {
                fprintf(stderr, "%s: failed to read child %d errno %d!\n", name, i, errno);
                exit(EXIT_FAILURE);
            }
            close(pipes[i][PIPE_FROM_C][PIPE_R]);
        }

        for (int i = 0; i < CHILD_NUM; i++)
        {
            waitpid(procs[i], &status[i], 0);
        }
        //fflush(stdout);
        fprintf(stderr, "%s: parent 3\n", name);

        for (int i = 0; i < CHILD_NUM; i++)
        {
            fprintf(stderr, "%s: results %d %s\n", name, i, results[i]);
        }
    }

    //TODO: Handle the result
    //TODO: print the result
    //TODO: optional: make cool print
    fprintf(stderr, "closing\n");
    exit(EXIT_SUCCESS);
}
