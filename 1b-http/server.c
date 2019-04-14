/**
 * @file server.c
 * @author Harrys Kavan <e1529309@student.tuwien.ac.at>
 * @date 12.04.2019
 *
 * @brief HTTP Server Software
 * 
 * This Program allows to offer files that can be fetched by HTTP.
 **/
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

static char *name = NULL;

static char *port = NULL;
static char *defaultPort = "8080";
static char *indexFileDefault = "index.html";
static char *indexFile = NULL;
static char *docRoot = NULL;
static char *reqPath = NULL;
static char *resHeader = NULL;
static char *resStatusCode = NULL;
static char *resMsg = NULL;
static char *reqUrl = NULL;
static int sockfd;

volatile sig_atomic_t done = 0;

/**
 * @brief Sets done to one to interrupt next loop.
 * @details This function will set done to 1, so that the after the handling of the last client request, the loop will be broken and the programm will exit instead of waiting for a new client.
 */
void term (int signum) {
    done = 1;
}

/**
 * @brief clean up function.
 * @details This function will clean up all remaining allocations.
 */
void cleanUp() {
    free(reqPath);
    free(resHeader);
}

/**
 * Mandatory usage function.
 * @brief This function writes helpful usage information about the program to stderr.
 */
void usage() {
    cleanUp();
    printf("SYNOPSIS\n\tserver [-p PORT] [-i INDEX] DOC_ROOT\nEXAMPLE\n\tserver -p 1280 -i index.html /Documents/my_website/\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Reads in all arguments and parses them.
 * @details Attempts to map all given arguments to the needed variables and pointers.
 * @param argc Number of arguments..
 * @param argv Argument Vector.
 */
void readArgs(int argc, char **argv) {
    int opt;
    while((opt = getopt(argc, argv, "p:i:")) != -1) {
        switch (opt) {
            case 'p':
                port = optarg;
                break;
            case 'i':
                //indexFile = optarg;
                indexFileDefault = optarg;
                break;
            default:
                usage();
                break;
        }
    }

    indexFile = indexFileDefault;

    if(port == NULL)
        port = defaultPort;

    char *endpnt;
    int portnr = strtol(port, &endpnt, 10);
    if((endpnt == '\0') || ((portnr < 1) || (portnr > 49151))) {
        fprintf(stderr, "%s: invalid port number!\n", name);
        usage();
    }

    if (optind >= argc) {
        fprintf(stderr, "%s: Missing arguments\n", name);
        usage();
    }

    docRoot = argv[optind];
}

/**
 * @brief Validate all given arguments.
 * @details Function will go throu all given arguments and attempt to validate them accordingly to the given requirements for this task.
 */
void validateArgs(void) {
}

/**
 * @brief Connect to Server over Socket.
 * @details This function will create a socket, bind it, listen on it and start accepting connections.
 * @return created Socket ID.
 */
void createConnection(void) {
    struct addrinfo hints, *ai;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo( NULL , port, &hints, &ai); 
    if (res != 0) {
        fprintf(stderr, "%s: Error getting address!\n", name);
        freeaddrinfo(ai);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    sockfd = socket(ai->ai_family, ai->ai_socktype,ai->ai_protocol);
    if (sockfd < 0) {
        fprintf(stderr, "%s: Error creating socket!\n", name);
        freeaddrinfo(ai);
        cleanUp();
        exit(EXIT_FAILURE);
    }
    if ( bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        fprintf(stderr, "%s: Error binding to socket!\n", name);
        freeaddrinfo(ai);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(ai);

    if (listen(sockfd, 1) < 0) {
        fprintf(stderr, "%s: Error while listeing on socket!\n", name);
        cleanUp();
        exit(EXIT_FAILURE);
    }

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
            fprintf(stdout, "%s: unexpected client disconnect.\n", name);
            return 0;
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
 * @details This function goes through the first line of a response Header and will check if it complies with requirements given for this task. 
 * @param pnt A pointer pointing to the first Line of a response Header.
 */
int checkFirstLine(char *line) {

    int lineLen = strlen(line);
    // Check if begin of line equals "GET"
    char get[5];
    memcpy(get, &line[0], 4);
    get[4] = '\0';
    if((strcmp(get, "GET ")) != 0) {
        resStatusCode = "501 ";
        resMsg  = "(Not implemented)\r\n";
        return -1;
    }

    // Check if reqestPath exists.
    if((strncmp(line+4, "/", 1)) != 0) {
        fprintf(stdout, "%s: Protocol error!\n", name);
        cleanUp();
        exit(2);
    }

    // Check if end of line equals "HTTP/1.1"
    char HTTPVersion[10];
    memcpy(HTTPVersion, &line[lineLen-9], 9);
    HTTPVersion[9] = '\0';
    if((strcmp(HTTPVersion, " HTTP/1.1")) != 0) {
        fprintf(stderr, "%s: Protocol error!\n", name);
        cleanUp();
        exit(2);
    }

    reqUrl = strndup(line+4, lineLen-9-4);
    if(reqUrl == NULL) {
        fprintf(stderr, "%s: Memory error!\n", name);
        cleanUp();
        exit(EXIT_FAILURE);
    }

    int reqUrlLen = strlen(reqUrl);
    int reqPathLen = strlen(docRoot); 
    if(reqUrl[reqUrlLen-1] == '/') { // /folder/
        indexFile = indexFileDefault;
        reqPathLen += reqUrlLen;
        reqPathLen += strlen(indexFile);
    } else { // /file
        indexFile = reqUrl;
        reqPathLen += reqUrlLen;
    }
    reqPath = (char *)malloc(reqPathLen+1);
    if(reqPath == NULL) {
        fprintf(stderr, "%s: Memory error!\n", name);
        free(reqUrl);
        reqUrl = NULL;
        cleanUp();
        exit(EXIT_FAILURE);
    }
    memset(reqPath, 0, reqPathLen+1);
    memcpy(reqPath, docRoot, strlen(docRoot));
    if(reqUrl[reqUrlLen-1] == '/') {
        memcpy(reqPath+strlen(docRoot), reqUrl, reqUrlLen);
        memcpy(reqPath+strlen(docRoot)+reqUrlLen, indexFile, strlen(indexFile));
    } else {
        memcpy(reqPath+strlen(docRoot), indexFile, strlen(indexFile));
    }

    free(reqUrl);

    if (access(reqPath, R_OK) == -1) {
        fprintf(stdout, "%s: Requested file '%s' does not exist!\n", name, reqPath);
        resStatusCode = "404 ";
        resMsg = "(Not Found)\r\n";
        return -1;
    }

    struct stat path_stat;
    stat(reqPath, &path_stat);
    if(S_ISDIR(path_stat.st_mode)){
        fprintf(stdout, "%s: Requested file '%s' is a folder!\n", name, reqPath);
        resStatusCode = "404 ";
        resMsg = "(Not Found)\r\n";
        return -1;
    }

    return 0;
}

/**
 * Program entry point.
 * @brief Program starts here.
 * @details The Program will first handle and validate all arguments and will then start offering its service.
 * The server can be stopped by SIGINT, SIGTERM.
 * @param argc The argument counter.
 * @param argv The argument vector.
 * @return Returns EXIT_SUCCESS.
 */
int main (int argc, char **argv) {
    int con;
    name = argv[0];

    readArgs(argc, argv);
    validateArgs();
    createConnection();

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    while(!done) {
        con = accept(sockfd, NULL, NULL);

        if(con < 0) {
            fprintf(stderr,"%s: Error starting network service!\n", name);
            cleanUp();
            exit(EXIT_FAILURE);
        }

        char *line = NULL;
        int linelen = 0;
        int firstLine = 0;
        int headerError = 0;

        for(;;) { // Read Request
            if ((linelen = fgetline(con, &line)) < 1) { // TODO: maybe check *line
                break;
            }
            if (firstLine == 0) {
                headerError = checkFirstLine(line);
                firstLine = 1;
            }
            free(line);
            line = NULL;
        }
        free(line);

        if(headerError == -1) {

            time_t t = time(NULL);
            struct tm* tm_info;
            int tBufLen = 40;
            char date[tBufLen];
            memset(date, 0, tBufLen+1);
            tm_info = gmtime(&t);
            strftime(date, 38, "Date: %a, %d %b %y %H:%M:%S GMT\r\n", tm_info);

            char *HTTPVersion = "HTTP/1.1 ";
            char *conClose = "Connection: Close\r\n\r\n";
            int resHeaderLen = strlen(HTTPVersion) + strlen(resStatusCode) + strlen(resMsg) + strlen(date) + strlen(conClose);

            resHeader = (char *)malloc(resHeaderLen+1);
            if(resHeader == NULL) {
                fprintf(stderr, "%s: Memory error!", name);
                cleanUp();
                exit(EXIT_FAILURE);
            }
            memset(resHeader, 0, resHeaderLen+1);
            snprintf(resHeader, resHeaderLen+1, "%s%s%s%s%s", HTTPVersion, resStatusCode, resMsg, date, conClose);

            //SEND HEADER
            if (write(con, resHeader, strlen(resHeader)) != strlen(resHeader)) {
                fprintf(stderr, "%s: Error while sending request.\n", name);
                cleanUp();
                exit(EXIT_FAILURE);
            }

        } else {
            //DATE
            time_t t = time(NULL);
            struct tm* tm_info;
            int tBufLen = 40;
            char date[tBufLen];
            memset(date, 0, tBufLen+1);
            tm_info = gmtime(&t);
            strftime(date, 38, "Date: %a, %d %b %y %H:%M:%S GMT\r\n", tm_info);

            //CONTENT LENGTH
            FILE *f = fopen(reqPath, "rb");
            if (f == NULL) {
                fprintf(stderr, "%s: Error reading file!\n", name); // 404?
                cleanUp();
                exit(EXIT_FAILURE);
            }

            fseek( f, 0L, SEEK_END );
            long contentLen = ftell(f); //should always be > 0 because checkFirstLine()
            rewind(f);

            int contentLenInt = snprintf( NULL, 0, "%ld", contentLen); // Assuming file is not larger then 3.2 MB
            char resContentSize[(14 + contentLenInt + 3)];
            memset(resContentSize, 0, 14 + contentLenInt + 3);
            snprintf(resContentSize, (14 + contentLenInt + 3), "Content-Size: %ld\r\n", contentLen);

            // OTHER
            resStatusCode = "200 ";
            resMsg = "OK\r\n";
            char *HTTPVersion = "HTTP/1.1 ";
            char *conClose = "Connection: Close\r\n\r\n";

            // Build Header
            int resHeaderLen = strlen(HTTPVersion) + strlen(resStatusCode) + strlen(resMsg) + strlen(date) + strlen(resContentSize) + strlen(conClose);
            resHeader = (char *)malloc(resHeaderLen+1);
            if(resHeader == NULL) {
                fprintf(stderr, "%s: Memory error!", name);
                cleanUp();
                exit(EXIT_FAILURE);
            }
            memset(resHeader, 0, resHeaderLen+1);
            snprintf(resHeader, resHeaderLen+1, "%s%s%s%s%s%s", HTTPVersion, resStatusCode, resMsg, date, resContentSize, conClose);

            //SEND HEADER
            if (write(con, resHeader, strlen(resHeader)) != strlen(resHeader)) {
                fprintf(stderr, "%s: Error while sending request.\n", name);
                cleanUp();
                exit(EXIT_FAILURE);
            }

            //READ FILE
            char *resBody;
            size_t n = 0;
            int ch;

            resBody = malloc(contentLen+1);
            if(resBody == NULL) {
                fprintf(stderr, "%s: Memory error!\n", name);
                cleanUp();
                exit(EXIT_FAILURE);
            }

            while((ch = fgetc(f)) != EOF) {
                if(ch < 0) {
                    fprintf(stderr,"%s: Error reading file!\n", name);
                    cleanUp();
                    exit(EXIT_FAILURE);
                }
                //fprintf(stderr, "%c", ch);
                resBody[n++]  = (char)ch;
            }
            resBody[n]  = '\0';
            fclose(f);

            //SEND FILE
            char* resBodyPnt = resBody;
            ssize_t ret;
            size_t toWrite = contentLen;
            while(toWrite > 0) {
                do {
                    ret = write(con, resBodyPnt, toWrite);
                } while ((ret < 0) && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
                if(ret < 0) {
                    fprintf(stderr,"%s: Error transmitting data!\n", name);
                    cleanUp();
                    exit(EXIT_FAILURE);
                }
                toWrite -= ret;
                resBodyPnt += ret;
            }

            shutdown(con, SHUT_WR);
            //fprintf(stderr, "%s: done writing data!\n", name);

        }
        close(con);

    }
    shutdown(con, SHUT_RDWR);
    close(con);
    cleanUp();

}

