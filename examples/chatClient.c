/*
 * This file implements an echo client.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

#include "coco.h"

#define MAXLINE 1024

static int	open_client(char *hostname, char *port);
void unix_error(const char *msg) {
    int errnum = errno;
    fprintf(stderr, "%s (%d: %s)\n", msg, errnum, strerror(errnum));
    exit(EXIT_FAILURE);
}
/*
 * Requires:
 *   argv[1] is a string representing a host name, and argv[2] is a string
 *   representing a TCP port number (in decimal).
 *
 * Effects:
 *   Opens a connection to the specified server.  Then, repeats the following
 *   actions.  First, reads a line from stdin, and writes that line to the
 *   server.  Second, reads a line from the server, and writes that line to
 *   stdout.  This loop stops when reading a line from stdin returns EOF. 
 */
struct host_port {
    char *host;
    char *port;
};

void echo(int clientfd) {
    char buf[MAXLINE];
    while (1) {
        coco_yield();
        ssize_t n = recv(clientfd, buf, MAXLINE, MSG_DONTWAIT);
        if (n == 0) {
            break;
        }
        if(n == -1) {
            if(errno == EAGAIN) {
                continue;
            } else {
                unix_error("recv error");
            }
        }
        write(fileno(stdout), buf, n);
    }
    coco_exit(0);
}

void sendstdin(int clientfd) {
    char buf[MAXLINE];
    struct pollfd fds[1];
    fds[0].fd = fileno(stdin);
    fds[0].events = POLLIN;
    while (1) {
                coco_yield();
        poll(fds, 1, 0);
        if (!(fds[0].revents & POLLIN)) {
            continue;
        }
        ssize_t n = read(fileno(stdin), buf, MAXLINE);
        n = write(clientfd, buf, n);

        
    }
    coco_exit(0);
}

void kernal(struct host_port *p) {
	long int clientfd;
	char *host, *port;

	host = p->host;
    port = p->port;
	clientfd = open_client(host, port);
	if (clientfd == -1) {
		unix_error("open_client Unix error");
	} else if (clientfd == -2) {
		unix_error("open_client DNS error");
	}
    printf("Connected to %s:%s\n", host, port);

    int tid = add_task(AS_COROUTINE(echo), (void *)clientfd);
    add_task(AS_COROUTINE(sendstdin), (void *)clientfd);
    coco_waitpid(tid, NULL, 0);

    close(clientfd);
    coco_exit(0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    static struct host_port port;
    port.host = argv[1];
    port.port = argv[2];
    coco_start((coroutine)kernal, &port);
    return 0;
}

/*
 * Requires:
 *   hostname points to a string representing a host name, and port points to
 *   a string representing a TCP port number.
 *
 * Effects:
 *   Opens a TCP connection to the server at <hostname, port>, and returns a
 *   file descriptor ready for reading and writing.  Returns -1 and sets
 *   errno on a Unix error.  Returns -2 on a DNS (getaddrinfo) error.
 */
static int
open_client(char *hostname, char *port)
{
	struct addrinfo *ai, hints, *listp;
	int fd;

	/*
	 * Prevent "unused parameter/variable" warnings.  REMOVE THESE
	 * STATEMENTS!
	 */
	(void)hostname;
	(void)port;

	// Initialize the hints that should be passed to getaddrinfo().
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;  // Open a connection ...
	hints.ai_flags = AI_NUMERICSERV;  // ... using a numeric port arg.
	hints.ai_flags |= AI_ADDRCONFIG;  // Recommended for connections

	/*
	 * Use getaddrinfo() to get the server's address list (&listp).  On
	 * failure, return -2.
	 */
	// REPLACE THIS.
	if(getaddrinfo(hostname, port, &hints, &listp)) printf("bad get addr info\n");


	/*
	 * Iterate over the address list for one that can be successfully
	 * connected to.
	 */
	for (ai = listp; ai != NULL; ai = ai->ai_next) {
		/*
		 * Create a new socket using ai's family, socktype, and
		 * protocol, and assign its descriptor to fd.  On failure,
		 * continue to the next ai.
		 */
		
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if(fd == -1) {
			continue;
		}
		/*
		 * Try connecting to the server with ai's addr and addrlen.
		 * Break out of the loop if connect() succeeds.
		 */
		int err = connect(fd, ai->ai_addr, ai->ai_addrlen);
		if(err) {
			printf("clientside err %dn", err);
		} else {
			break;
		}


		/*
		 * Connect() failed.  Close the descriptor and continue to
		 * the next ai.
		 */
		if (close(fd) == -1)
			unix_error("close");
	}

	// Clean up.  Avoid memory leaks!
	freeaddrinfo(listp);
	if (ai == NULL) {
		// All connection attempts failed.
		return (-1);
	} else {
		// The last connect succeeded.
		return (fd);
	}
}