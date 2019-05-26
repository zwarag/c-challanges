/**
 * @file intmul.c
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 26.05.2019
 * 
 * @brief Large Integer Multiplikation by fork
 * 
 * @detail: allows to multiply large hexadecimal numbers of length that is within the power of two.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define CHILD_NUM 4 /*!< maximum child number */
#define BUF_SIZE 4  /*!< standard buf size */

#define PIPE_R 0
#define PIPE_W 1
#define PIPE_TO_C 0
#define PIPE_FROM_C 1
static int pipes[CHILD_NUM][2][2]; /*!< proc,pipe-{from|to}-c,{read|write} */

static char *name = NULL;                                   /*!< program name */
static char *strNum1 = NULL;                                /*!< first Number as string */
static char *strNum2 = NULL;                                /*!< second Numer as string */
static int inputLength = 0;                                 /*!< input Length of strNum1 and strNum2 */
static char *Al = NULL;                                     /*!< number Al */
static char *Ah = NULL;                                     /*!< number Ah */
static char *Bl = NULL;                                     /*!< Number Bl */
static char *Bh = NULL;                                     /*!< Number Bh */
static char *results[CHILD_NUM] = {NULL, NULL, NULL, NULL}; /*!< results of multiplicaiton Ah * Bh * 100 ; Ah * Bl * 10 : Al * Bh * 10 ; Ab * Bh */
static size_t chars[CHILD_NUM];                             /*!< length of chars in results */

/**
 * @brief clean up function.
 * @details This function will clean up all remaining allocations.
 * @return void
 * @param void
 */
void cleanUp(void)
{
    free(strNum1);
    free(strNum2);
}

/**
 * @brief clean up results function.
 * @details This function will clean up all remaining allocations of the results array.
 * @return void
 * @param void
 */
void cleanUpResults(void)
{
    for (int i = 0; i < CHILD_NUM; i++)
        free(results[i]);
}

/**
 * @brief Reads in all arguments and parses them.
 * @details Assures that no flags and arguments have been supplied. 
 * @param argc Number of arguments..
 * @param argv Argument Vector.
 * @return void
 */
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

/**
 * @brief Reads in one line of characters from stdin
 * @details Dynamically allocated buffer that can read one very long line from stdin. It does not assure that the input is usable as hexadecimal number.
 * @param void
 * @return (char *) pointing to the beginning of the memory wher the line is stored.
 */
char *readString(void)
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
            free(from);
            return NULL;
        }
        strcat(str, buf);
        char *pos;
        if ((pos = strchr(buf, '\n')) != NULL)
            break;
    }
    if (ferror(stdin))
    {
        free(str);
        return NULL;
    }
    return str;
}

/**
 * @brief parses a char (hex) to an int (dec)
 * @details Gets a char, interprets it as a hexadecimal value and returns its decimal value as an unsigned integer
 * @param char c char that should get converted
 * @return unsigned int value in decimal of the hexadecimal interpretation of char
 */
unsigned int parseChar(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return 10 + c - 'a';
    if ('A' <= c && c <= 'F')
        return 10 + c - 'A';
    cleanUp();
    cleanUpResults();
    exit(EXIT_FAILURE);
}

/**
 * @brief parses a int (dec) to an char (hex)
 * @details Gets a integer, interprets converts it the hexadecimal represenation stored as a char
 * @param  unsigned int that should get converted
 * @return char c value in hexadecimal of the decimal interpretation of int
 */
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
        return '9';
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
        cleanUp();
        cleanUpResults();
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief sets pointers for the Arithmetic operations.
 * @details Ah, Al, Bh, Bl will allways be accesed through [0]. So it actually just points to the values that we want to work with.
 * @param  void
 * @return void
 */
void generatePointers(void)
{
    Ah = strNum1;
    Al = strNum1 + ((inputLength - 1) / 2);
    Bh = strNum2;
    Bl = strNum2 + ((inputLength - 1) / 2);
}

/**
 * Program entry point.
 * Formula (Ah * Bh * 100) + (Ah * Bl * 10) + (Al * Bh * 10) + (Al * Bl)
 * @brief Calculates the multiplicaiton of two hexadecimal numbers A * B
 * @details Program first reads two large numbers, then splits them up to calculate them by the formula shown above by forkig itself until no forthuer split is possible.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS.
 */
int main(int argc, char **argv)
{
    // Handle getopt
    getArgs(argc, argv);

    // Read into dynamic buffers
    strNum1 = readString();
    if (strNum1 == NULL)
    {
        exit(EXIT_FAILURE);
    }
    strNum2 = readString();
    if (strNum2 == NULL)
    {
        free(strNum1);
        exit(EXIT_FAILURE);
    }

    //TODO: check equal length
    if (strlen(strNum1) != strlen(strNum2))
    {
        cleanUp();
        exit(EXIT_FAILURE);
    }
    inputLength = strlen(strNum1);

    if (strlen(strNum1) < 2 || strlen(strNum2) < 2)
    {
        cleanUp();
        exit(EXIT_FAILURE);
    }

    // check length is one and multiply
    if (strlen(strNum1) == 2 && strlen(strNum2) == 2)
    {
        fprintf(stdout, "%1x", parseChar(strNum1[0]) * parseChar(strNum2[0]));
        cleanUp();
        exit(EXIT_SUCCESS);
    }

    //TODO: Fork work to childs
    pid_t procs[CHILD_NUM] = {-1, -1, -1, -1};
    int childs = 0;
    int status[CHILD_NUM];

    for (int i = 0; i < CHILD_NUM; i++)
    {
        if (pipe(pipes[i][PIPE_TO_C]) == -1)
        {
            cleanUp();
            exit(EXIT_FAILURE);
        }
        if (pipe(pipes[i][PIPE_FROM_C]) == -1)
        {
            cleanUp();
            exit(EXIT_FAILURE);
        }
    }

    do
    {
        procs[childs] = fork();
        if (procs[childs] == -1)
        {
            cleanUp();
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

        execlp(name, name, NULL);
        fflush(stdout);
        close(pipes[childs][PIPE_FROM_C][PIPE_W]);
        close(pipes[childs][PIPE_TO_C][PIPE_R]);
    }
    else
    { // parent

        // close pipes to other childs
        for (int i = 0; i < CHILD_NUM; i++)
        {
            close(pipes[i][PIPE_FROM_C][PIPE_W]);
            close(pipes[i][PIPE_TO_C][PIPE_R]);
        }

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

        //read from pipes
        for (int i = 0; i < CHILD_NUM; i++)
        {
            FILE *fd = fdopen(pipes[i][PIPE_FROM_C][PIPE_R], "r");
            if (fd == NULL)
            {
                cleanUp();
                exit(EXIT_FAILURE);
            }
            if (getline(&results[i], &chars[i], fd) == -1)
            {
                fclose(fd);
                while (i > 0)
                {
                    close(pipes[i][PIPE_FROM_C][PIPE_R]);
                    free(results[i]);
                    i--;
                }
                free(results[i]);
                for (int i = 0; i < CHILD_NUM; i++)
                    close(pipes[i][PIPE_FROM_C][PIPE_R]);
                cleanUp();
                exit(EXIT_FAILURE);
            }
            fclose(fd);
            close(pipes[i][PIPE_FROM_C][PIPE_R]);
        }

        // wait for child to finish
        for (int i = 0; i < CHILD_NUM; i++)
        {
            waitpid(procs[i], &status[i], 0);
        }

        // exit if there was an error within a child
        for (int i = 0; i < CHILD_NUM; i++)
        {
            if (status[i] != 0)
            {
                cleanUp();
                cleanUpResults();
                exit(EXIT_FAILURE);
            }
        }

        // handle the result
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
        char buf[(2 * inputLength) + 1];
        memset(buf, 0, (2 * inputLength) + 1);
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
                if (chars[1] > 0)
                {
                    c1_val = parseChar(results[1][chars[1] - 1]);
                    chars[1]--;
                }
                else
                {
                    c1_val = 0;
                }
                if (chars[2] > 0)
                {
                    c2_val = parseChar(results[2][chars[2] - 1]);
                    chars[2]--;
                }
                else
                {
                    c2_val = 0;
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
        else
            bufPnt--;

        // print the result
        int h = 1;
        while (bufPnt != buf)
        {
            if (bufPnt[0] != '0' || h == 0)
            {
                fprintf(stdout, "%c", bufPnt[0]);
                h = 0;
            }
            bufPnt--;
        }
        fprintf(stdout, "%c", bufPnt[0]);
    }
    fflush(stdout);
    cleanUp();
    cleanUpResults();
    exit(EXIT_SUCCESS);
}
