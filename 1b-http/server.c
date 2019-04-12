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

static char *name;

static int opt;
static char *port;
static char *indexFile;
static char *docRoot = NULL;
static char *readPath = NULL;
static char *reqPath;
static char *resHeader;
static char *resStatusCode;
static char *resMsg;
static int sockfd;

int main() {
    exit(EXIT_SUCCESS);
}
