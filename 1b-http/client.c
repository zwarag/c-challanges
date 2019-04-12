/**
 * @file client.c
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 11.11.2018
 * 
 * @brief HTTP Client Software
 * 
 * This Program allows to load Files from HTTP Server supporting HTTP/1.1.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static char *name;

static int opt;
static int fileFlag = 0;
static int dirFlag = 0;
static char *port;
static char *filePath;
static char *dirPath;
static char *url;
static char *req;
static char *host;

/**
 * @brief clean up function.
 * @details This function will clean up all remaining allocations.
 */
void cleanUp() {
    if(port == NULL)
        free(port);
    if(host == NULL)
        free(host);
    if(req == NULL)
        free(req);
}

/**
 * Mandatory usage function.
 * @brief This function writes helpful usage information about the program to stderr.
 */
void usage() {
    cleanUp();
    fprintf(stderr, "SYNOPSIS\n\t\tclient [-p PORT] [ -o FILE | -d DIR ] URL\n\tEXAMPLE\n\t\tclient http://pan.vmars.tuwien.ac.at/osue/\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief find the last occurence of a character in a string.
 * @details function will jump to end of the omitted string and will search for the last occurence of the searched character.
 * @param pnt A pointer pointing at a string.
 * @param search character to look for.
 * @return The index of the last character that matches the search. 0 if none is found.
 */
int findLastOccurence(char *pnt, char search) {
    int pntLen = strlen(pnt)-1;
    for(int i = pntLen; i > 0; i--) {
        if((i < 12))
            break;
        if(pnt[i] == search) {
            if (i == pntLen) 
                break;
            else
                return ++i;
        }
    }
    return 0;
}

/**
 * @brief Reads in all arguments and parses them.
 * @details Attempts to map all given arguments to the needed variables and pointers.
 * @param argc Number of arguments..
 * @param argv Argument Vector.
 */
void readArgs(int argc, char **argv) {
    while((opt = getopt(argc, argv, "p:o:d:")) != -1) {
        switch(opt) {
            case 'p':
                port = strdup(optarg);
                char *endpnt;
                int portnr = strtol(port, &endpnt, 10);
                if((endpnt == '\0') || ((portnr < 1) || (portnr > 49151))) {
                    fprintf(stderr, "%s: invalid port number!\n", name);
                    usage();
                }
                break;
            case 'o':
                filePath = strdup(optarg);
                fileFlag = 1;
                break;
            case 'd':
                dirPath = strdup(optarg);
                dirFlag = 1;
                break;
            default: /* '?' */
                usage();
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "%s: Missing arguments\n", name);
        usage();
    }

    if(port == NULL)
        port = strdup("80");

    url = strdup(argv[optind]);
    if(filePath != NULL && dirPath != NULL) {
        fprintf(stderr, "%s: Both, file and direcotry paths are set. Remove one!\n", name);
        usage();	
    }
    if(dirFlag == 1) {
        int lastSlash = findLastOccurence(url, '/');	
        if(lastSlash == 0) {
            filePath = "index.html";
        } else {
            filePath = url+lastSlash;
        }

    }
}

/**
 * @brief find the n'th occurence of a character in a string.
 * @details function will iterate the omitted string and will search for the n'th occurence of the searched character.
 * @param pnt A pointer pointing at a string.
 * @param search character to look for.
 * @param n n'th occurence.
 * @return The index of the n'th character that matches the search. -1 if none is found.
 */
int findNthOccurence(char *s, char c, int n) {
    int occ = 0;
    for(int i = 0; i < strlen(s); i++) {
        if(s[i] == c) {
            occ++;
            if(n == occ) 
                return i;
        }
    }
    return -1;
}

/**
 * @brief Extract the file from the URL.
 * @details Returns a pointer from where the file path starts.
 * @return A pointer pointing to the beginning of the filePath.
 */
char *fileFromURL() {
    char *retVal;
    int from = findNthOccurence(url, '/', 3);	
    if(from < 0) {
        fprintf(stderr, "The provided URL is not conform!\n");
        usage();
    }
    retVal = url+from;

    return retVal;
}

/**
 * @brief Extract the host from the URL.
 * @details Sets the global host variable to the omitted Host.
 */
void hostFromURL() {
    int from = findNthOccurence(url, '/', 2);
    int to   = findNthOccurence(url, '/', 3);
    if((from < to) && (from < 0 || to < 1)) {
        fprintf(stderr, "The provided URL is not conform!\n");
        usage();
    }
    host = strndup(url+from+1, to-from-1);
}

/**
 * @brief Build the request header.
 * @details When all data is set. This function will merge all components into the global 'req' variable.
 */
void buildRequest() {
    char *method = strdup("GET");
    char *file = fileFromURL();
    char *HTTPVersion = strdup("HTTP/1.1");
    hostFromURL();

    if(host == NULL) {
        fprintf(stderr, "Cannot find host in arguments\n");
        free(method);
        free(HTTPVersion);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    if(method == NULL || file == NULL || HTTPVersion == NULL || host == NULL) {
        fprintf(stderr, "The provided URL is not valid!\n");
        free(method);
        free(HTTPVersion);
        usage();
    }

    size_t size = 0;
    size = snprintf(NULL, 0, "%s %s %s\r\nHost: %s\r\n\r\n", method, file, HTTPVersion, host);
    if( size > 0 ) {
        req = (char *)malloc(size + 1 + (19*sizeof(char)));
        if(req == NULL) {
            fprintf(stderr, "%s: Memory error!", name);
            cleanUp();
            exit(EXIT_FAILURE);
        }
        snprintf(req, size+1+ (19*sizeof(char)), "%s %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n", method, file, HTTPVersion, host);
    } else {
        fprintf(stderr, "%s: Cannot get size of request String\n", name);
        free(method);
        free(HTTPVersion);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    if(method != NULL)
        free(method);
    if(HTTPVersion != NULL)
        free(HTTPVersion);	

}

/**
 * @brief Connect to Server over Socket.
 * @details This function will create a socket, bind it, listen on it and start accepting connections.
 * @return created Socket ID.
 */
int connectToServer() {
    int connection;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* stream socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */

    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0 ) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        cleanUp();
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        connection = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (connection == -1) {
            fprintf(stderr,"socket error");
            cleanUp();
            exit(EXIT_FAILURE);
        }

        if (connect(connection, rp->ai_addr, rp->ai_addrlen) != -1) {
            break; /* Success */
        }

        close(connection);
    }

    if (rp == NULL) { /* No address succeeded */
        fprintf(stderr,"%s: Could not connect to host\n", name);
        cleanUp();
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);

    return connection;
}

/**
 * @brief Getline from Socket.
 * @details By reading character by character, this function builds up a whole line and then returns it.
 * The Memory needed is dynamically allocated.
 * @param con socket id.
 * @param pnt an empty pointer pointing to the resulting line.
 * @return size of the buffer created to store the line.
 */
int fgetline(int con, char **pnt) {
    int bufferSize = 0; 
    int currentBufferSize = 0; 
    int linelen;
    char msg; 
    char *buffer = NULL; 
    char *newBuffer;

    for(;;) {
        linelen = read(con, &msg, 1);
        if (linelen < 1) {
            fprintf(stderr, "%s: Nothing was recived\n", name);
            free(buffer);
            cleanUp();
            exit(2);
        }
        if (msg == '\n')
            break;
        if ((bufferSize == 0) || (currentBufferSize == bufferSize)) { 
            bufferSize += 64; 
            newBuffer = realloc(buffer, bufferSize); 
            if (!newBuffer) { 
                fprintf(stderr, "%s: Error realloc memory!\n", name);	
                free(buffer);
                cleanUp();
                exit(EXIT_FAILURE);
            } 
            buffer = newBuffer; 
        } 
        buffer[currentBufferSize] = msg;
        ++currentBufferSize;
    } 

    if ((currentBufferSize > 0) && (buffer[currentBufferSize-1] == '\r'))
        --currentBufferSize;

    // when an empty line is omitted... we need to recheck.
    if ((bufferSize == 0) || (currentBufferSize == bufferSize)) { 
        bufferSize += 64; 
        newBuffer = realloc(buffer, bufferSize); 
        if (!newBuffer) { 
            fprintf(stderr, "%s: Error realloc memory!\n", name);	
            free(buffer);
            cleanUp();
            exit(EXIT_FAILURE);
        } 
        buffer = newBuffer; 
    } 

    buffer[currentBufferSize] = '\0';
    *pnt = buffer; 	
    return currentBufferSize;

}

/**
 * @brief Validates the first line of a response header.
 * @details This function goes throu the first line of a response Header and will check if it complies with requirements given for this task. 
 * @param pnt A pointer pointing to the first Line of a response Header.
 */
void checkFirstLine(char *pnt) {
    // Check if begin of line equals "HTTP/1.1"
    char HTTPVersion[9];
    memcpy(HTTPVersion, &pnt[0], 8);
    HTTPVersion[8] = '\0';
    if((strcmp(HTTPVersion, "HTTP/1.1")) != 0) {
        fprintf(stderr, "%s: Protocol error!\n", name);
        cleanUp();
        exit(2);
    }

    // Check if sencond part of line is a statuscode.
    char statuscode[4];
    memcpy(statuscode, &pnt[9], 3);
    statuscode[3] = '\0';
    int sc=0;
    char *endpnt = NULL;
    sc = strtol(statuscode, &endpnt, 10);
    if((sc < 100) || (sc > 511)) {
        fprintf(stderr, "%s: Protocol Error!\n", name);
        cleanUp();
        exit(2);
    }

    // Handle non 200 case
    if(sc != 200) {
        int ptnLen = strlen(pnt);
        char msg[ptnLen-13];
        fprintf(stderr, "%s: %s ", name, statuscode);
        if((&pnt[ptnLen]-&pnt[13]) > 1) {
            memcpy(msg, &pnt[13], ptnLen-13);
            msg[ptnLen-13] = '\0';
            fprintf(stderr, "%s", msg);
        }
        fprintf(stderr, "\n");
        cleanUp();
        exit(3);
    }	

}

/**
 * Program entry point.
 * @brief Program starts here.
 * @details The Program will first handle and validate all arguments and then it will attempt to fetch the requested file.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS.
 */
int main (int argc, char **argv) {
    int con;
    name = argv[0];
    readArgs(argc, argv);
    buildRequest();

    if((con = connectToServer()) == (-1)) {
        fprintf(stderr, "%s: Error while attempting to connect to Server.\n",name);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    if (write(con, req, strlen(req)+1) != strlen(req)+1) {
        fprintf(stderr, "%s: Error while sending request.\n", name);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    int linelen = 0;
    int firstLine = 0;
    for(;;) {
        if ((linelen = fgetline(con, &line)) <= 1)
            break;
        if (firstLine == 0)
            checkFirstLine(line);

        free(line);
        firstLine = 1;
    }
    free(line);


    if(fileFlag == 1 || dirFlag == 1) {

        int filePathLen = 0;
        int dirPathLen = 0;
        if(filePath != NULL)
            filePathLen = strlen(filePath);
        if(dirPath != NULL)
            dirPathLen = strlen(dirPath);
        int bufferSize = filePathLen + dirPathLen + 2;
        char *writePath;
        writePath = (char *)malloc(bufferSize);
        if(writePath == NULL) {
            fprintf(stderr, "%s: Memory error!", name);
            cleanUp();
            exit(EXIT_FAILURE);
        }
        memset(writePath, 0, bufferSize);
        if(dirPathLen > 0) {
            strncat(writePath, dirPath, dirPathLen+1);
            strncat(writePath, "/\0", 2);
        }
        if(filePathLen < 1) {
            fprintf(stderr, "%s: this should never happen\n", name);
            exit(EXIT_FAILURE);
        }
        strncat(writePath, filePath, filePathLen);
        if(writePath == NULL) {
            fprintf(stderr, "%s: Error with Memory", name);
            exit(EXIT_FAILURE);
        }

        FILE *f = fopen(writePath, "w");
        if (f == NULL) {
            fprintf(stderr, "%s: Error opening file!\n", name);
            exit(EXIT_FAILURE);
        }

        FILE *sockfile = (FILE *) fdopen(con, "r");
        int ch;
        while ((ch = fgetc(sockfile)) != EOF) {
            fprintf(f, "%c", ch);
        }

        close(con);
        fclose(f);
        fclose(sockfile);

        free(writePath);
    } else {

        FILE *sockfile = (FILE *) fdopen(con, "r");
        int ch;

        while ((ch = fgetc(sockfile)) != EOF) {
            fprintf(stdout, "%c", ch);
        }
    }

    cleanUp();
    exit(EXIT_SUCCESS);
}

