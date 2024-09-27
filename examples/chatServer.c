/*
 * This file implements an iterative echo server.
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>

#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "coco.h"

static int open_listen(int port);

/*
 * Requires:
 *   "connfd" is a connected socket.
 *
 * Effects:
 *   Echoes lines of text until the client closes the connection.
 */
#define MAXLINE 1024

void unix_error(const char *msg) {
    int errnum = errno;
    fprintf(stderr, "%s (%d: %s)\n", msg, errnum, strerror(errnum));
    coco_exit(EXIT_FAILURE);
}

enum app_state { APP_STATE_NONE, APP_STATE_ECHO };

enum app_state processCommand(int connfd, char *buf, ssize_t n,
                              enum app_state state) {
    if (n >= 4 && strncmp(buf, "exit", 4) == 0) {
        printf("server closing connection\n");
        close(connfd);
        coco_exit(0);
    }
    if (n >= 4 && strncmp(buf, "echo", 4) == 0) {
        write(connfd, "In Echo Mode\n", 13);
        return APP_STATE_ECHO;
    }
    if (n >= 4 && strncmp(buf, "help", 4) == 0) {
        // write(connfd, "Commands:\n\techo\n\texit\n", 24);
        write(connfd,
              "Commands:\n\techo - echo mode\n\texit - exit server\n\t"
              "help - print this message\n",
              75);
        return state;
    }
    write(connfd, "Unknown Command\n", 16);
    return state;
}

void writePrompt(int connfd, enum app_state state) {
    switch (state) {
    case APP_STATE_ECHO:
        write(connfd, "echo> ", 6);
        break;
    case APP_STATE_NONE:
        write(connfd, "> ", 3);
        break;
    }
}

void handle(int connfd) {
    ssize_t n;
    char buf[MAXLINE];
    enum app_state state = APP_STATE_NONE;
    writePrompt(connfd, state);

    while (1) {
        coco_yield();
        n = recv(connfd, buf, MAXLINE, MSG_DONTWAIT);

        if (n == 0) {
            processCommand(connfd, "exit", 4, state);
            break;
        }
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            unix_error("recv error");
        }
                    printf("server received %zd bytes\n", n);

        if (buf[0] == '$') {
            state = processCommand(connfd, buf + 1, n, state);
            writePrompt(connfd, state);
            continue;
        }

        switch (state) {
        case APP_STATE_ECHO:
            write(connfd, buf, n);

            break;
        case APP_STATE_NONE:
            write(connfd,
                  "No Mode Selected\nRun '$help' for a list of commands\n", 54);
            break;
        }
        writePrompt(connfd, state);
    }
}

/*
 * Requires:
 *   argv[1] is a string representing an unused TCP port number (in decimal).
 *
 * Effects:
 *   Opens a listening socket on the specified TCP port number.  Runs forever
 *   accepting client connections.  Echoes lines read from a connection until
 *   the connection is closed by the client.  Only accepts a new connection
 *   after the old connection is closed.
 */
void kernal(int *port) {
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    int connfd, listenfd;
    char haddrp[INET_ADDRSTRLEN], host_name[NI_MAXHOST];

    listenfd = open_listen(*port);
    if (listenfd < 0)
        unix_error("open_listen error");
    while (true) {
        coco_yield();
        // zombs
        clientlen = sizeof(clientaddr);
        /*
         * Call Accept() to accept a pending connection request from
         * the client, and create a new file descriptor representing
         * the server's end of the connection.  Assign the new file
         * descriptor to connfd.
         */
        struct pollfd fds[1];
        fds[0].fd = listenfd;
        fds[0].events = POLLIN;
        poll(fds, 1, 0);
        if (!(fds[0].revents & POLLIN)) {
            continue;
        }

        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        // Use getnameinfo() to determine the client's host name.
        getnameinfo((struct sockaddr *)&clientaddr, clientlen, host_name,
                    NI_MAXHOST, NULL, 0, AI_NUMERICSERV | AI_ADDRCONFIG);
        /*
         * Convert the binary representation of the client's IP
         * address to a dotted-decimal string.
         */
        inet_ntop(AF_INET, &clientaddr.sin_addr, haddrp, INET_ADDRSTRLEN);
        printf("server connected to %s (%s)\n", host_name, haddrp);
        /*
         * Echo lines of text until the client closes its end of the
         * connection.
         */
        add_task(AS_COROUTINE(handle), (void *)(uintptr_t)connfd);
        // echo(connfd);
        // echo(connfd);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);
    coco_start((coroutine)kernal, &port);
    return (EXIT_SUCCESS);
}

/*
 * Requires:
 *   port is an unused TCP port number.
 *
 * Effects:
 *   Opens and returns a listening socket on the specified port.  Returns -1
 *   and sets errno on a Unix error.
 */
static int open_listen(int port) {
    struct sockaddr_in serveraddr;
    int listenfd, optval;

    // Set listenfd to a newly created stream socket.
    // REPLACE THIS.
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        printf("Bad open socket");
    }
    // Eliminate "Address already in use" error from bind().
    optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                   sizeof(int)) == -1)
        return (-1);
    memset(&serveraddr, 0, sizeof(serveraddr));
    /*
     * Set the IP address in serveraddr to the special ANY IP address, and
     * set the port to the input port.  Be careful to ensure that the IP
     * address and port are in network byte order.
     */
    // int addr = htonl(INADDR_ANY);
    // printf("%d\n", addr);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);
    serveraddr.sin_family = AF_INET;
    // Use bind() to set the address of listenfd to be serveraddr.
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof serveraddr)) {
        printf("Bad bind");
    }

    /*
     * Use listen() to ready the socket for accepting connection requests.
     * Set the backlog to 8.
     */
    if (listen(listenfd, 8)) {
        printf("Bad listen");
    }
    return (listenfd);
}