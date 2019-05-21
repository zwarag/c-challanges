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
    fprintf(stderr, "%s: ERROR '%c' not a number!\n", name, c);
    exit(EXIT_FAILURE);
}

char parseInt(int c)
{
    switch (c)
    {
    case 0:
        return '0';
    case 1:
        return '1';
    case 2:
        return '2';
    case 3:
        return '3';
    case 4:
        return '4';
    case 5:
        return '5';
    case 6:
        return '6';
    case 7:
        return '7';
    case 8:
        return '8';
    case 9:
        return '8';
    case 10:
        return 'a';
    case 11:
        return 'b';
    case 12:
        return 'c';
    case 13:
        return 'd';
    case 14:
        return 'e';
    case 15:
        return 'f';
    default:
        fprintf(stderr, "%s: ERROR '%c' not a number!\n", name, c);
        exit(EXIT_FAILURE);
    }
}

/**
 * @param void
 **/

void generatePointers(void)
{
    Ah = strNum1;
    Al = strNum1 + (inputLength / 2);
    Bh = strNum2;
    Bl = strNum2 + (inputLength / 2);
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

    // check length is one and multiply
    if (strlen(strNum1) == 2 && strlen(strNum2) == 2)
    {
        fprintf(stdout, "%1x", parseChar(strNum1[0]) * parseChar(strNum2[0]));
        exit(EXIT_SUCCESS);
    }

    //TODO: Fork work to childs
    pid_t procs[CHILD_NUM] = {-1, -1, -1, -1};
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

    generatePointers();
    if (strlen(Ah) == 1)
    {
        procs[0] = -2;
    }
    if (strlen(Al) == 1)
    {
        procs[1] = -2;
    }
    if (strlen(Bh) == 1)
    {
        procs[2] = -2;
    }
    if (strlen(Bl) == 1)
    {
        procs[3] = -2;
    }

    do
    {
        if (procs[childs] != -2)
        {
            procs[childs] = fork();
            if (procs[childs] == -1)
            {
                fprintf(stderr, "%s: failed to fork!\n", name);
                exit(EXIT_FAILURE);
            }
            if (procs[childs] == 0)
                break;
        }
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

        execlp(name, name, NULL);
        fflush(stdout);
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

        generatePointers();
        fprintf(stderr, "---\n%s%s%s%s---\n", Ah, Al, Bh, Bl);

        // Child 0: Ah * Bh
        generatePointers();
        while (Ah != Al)
        {
            dprintf(pipes[0][PIPE_TO_C][PIPE_W], "%c", Ah[0]);
            Ah++;
        }
        dprintf(pipes[0][PIPE_TO_C][PIPE_W], "\n");
        while (Bh != Bl)
        {
            dprintf(pipes[0][PIPE_TO_C][PIPE_W], "%c", Bh[0]);
            Bh++;
        }
        dprintf(pipes[0][PIPE_TO_C][PIPE_W], "\n");
        close(pipes[0][PIPE_TO_C][PIPE_W]);

        // Child 1: Ah * Bl
        generatePointers();
        while (Ah != Al)
        {
            dprintf(pipes[1][PIPE_TO_C][PIPE_W], "%c", Ah[0]);
            Ah++;
        }
        dprintf(pipes[1][PIPE_TO_C][PIPE_W], "\n");
        dprintf(pipes[1][PIPE_TO_C][PIPE_W], "%s", Bl);
        close(pipes[1][PIPE_TO_C][PIPE_W]);

        // Child 2: Al * Bh
        generatePointers();
        dprintf(pipes[2][PIPE_TO_C][PIPE_W], "%s", Al);
        while (Bh != Bl)
        {
            dprintf(pipes[2][PIPE_TO_C][PIPE_W], "%c", Bh[0]++);
            Bh++;
        }
        dprintf(pipes[2][PIPE_TO_C][PIPE_W], "\n");
        close(pipes[2][PIPE_TO_C][PIPE_W]);

        // Child 3: Al * Bl
        generatePointers();
        dprintf(pipes[3][PIPE_TO_C][PIPE_W], "%s", Al);
        dprintf(pipes[3][PIPE_TO_C][PIPE_W], "%s", Bl);
        close(pipes[3][PIPE_TO_C][PIPE_W]);

        fprintf(stderr, "%s: parent 2\n", name);
        fflush(stderr);

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
            //fclose(fd);
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
            //fprintf(stderr, "%s: results %d %s\n", name, i, results[i]);
        }

        chars[0] = strlen(results[0]);
        chars[1] = strlen(results[1]);
        chars[2] = strlen(results[2]);
        chars[3] = strlen(results[3]);
        int offset = inputLength - 1;
        int offsetHalf = (inputLength - 1) / 2;
        unsigned int c3_val = 0;
        unsigned int c2_val = 0;
        unsigned int c1_val = 0;
        unsigned int c0_val = 0;
        unsigned int retval = 0;
        unsigned int overflow = 0;
        char buf[(chars[0]) + 1];
        char *bufPnt = buf;
        while (chars[0] > 0 || chars[1] > 0 || chars[2] > 0 || chars[3] > 0)
        {
            if (offset > 0)
            {
                offset--;
                c0_val = 0;
            }
            else
            {
                if (chars[0] > 0)
                {
                    c0_val = parseChar(results[0][chars[0] - 1]);
                    chars[0]--;
                }
                else
                {
                    c0_val = 0;
                }
            }
            if (offsetHalf > 0)
            {
                offsetHalf--;
                c1_val = 0;
                c2_val = 0;
            }
            else
            {
                if (chars[2] > 0)
                {
                    c2_val = parseChar(results[2][chars[2] - 1]);
                    chars[2]--;
                }
                else
                {
                    c2_val = 0;
                }
                if (chars[1] > 0)
                {
                    c1_val = parseChar(results[1][chars[1] - 1]);
                    chars[1]--;
                }
                else
                {
                    c1_val = 0;
                }
            }
            if (chars[3] > 0)
            {
                c3_val = parseChar(results[3][chars[3] - 1]);
                chars[3]--;
            }
            else
            {
                c3_val = 0;
            }

            retval = c3_val + c2_val + c1_val + c0_val + overflow;
            overflow = retval / 16;
            sprintf(bufPnt, "%c", parseInt(retval % 16));
            bufPnt++;
        }
        if (overflow != 0)
            sprintf(bufPnt, "%c", parseInt(overflow % 16));
        while (bufPnt != buf)
        {
            fprintf(stdout, "%c", bufPnt[0]);
            bufPnt--;
        }
        fprintf(stdout, "%c", bufPnt[0]);
    }
    fflush(stdout);
    //TODO: Handle the result
    //TODO: print the result
    //TODO: optional: make cool print
    //fprintf(stderr, "closing\n");
    exit(EXIT_SUCCESS);
}
