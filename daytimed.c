/*
 * Copyright (c) 2020 Scott Bennett <scottb@fastmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Implements a basic daytime daemon - RFC 867
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int debug = 0;
static const char *timeformat = "%a %b %e %H:%M:%S %Z %Y\n";

#define ADDR		INADDR_ANY
#define DEBUG_ADDR	INADDR_LOOPBACK
#define PORT		13
#define DEBUG_PORT	13013
#define _PW_USER	"_identd"
#define _PW_DIR		"/var/empty"

#define DPRINTF(x...) do { if (debug) printf(x); } while (0)

static void
usage(void)
{
	printf("daytimed [-d]\n");
	exit(1);
}

/*
 * Handler for SIGCHLD
 */
static void
kidhandler(int signum)
{
	waitpid(WAIT_ANY, NULL, WNOHANG);
}

/*
 * Get the current date and time and write a human-readable string to buf.
 */
void
getthetime(char *buf, const size_t maxlen)
{
	if (buf == NULL) {
		errx(1, "getthetime: buf is null");
	}
	if (maxlen < 2) {
		errx(1, "getthetime: maxlen is too short");
	}

	time_t tval;
	if (time(&tval) == -1) {
		err(1, "time failed");
	}

	struct tm *timeptr = localtime(&tval);
	if (timeptr == NULL) {
		err(1, "time conversion error");
	}

	size_t count = strftime(buf, maxlen, timeformat, timeptr);
	if (count == 0) {
		err(1, "couldn't write time string to buffer");
	}
}

/*
 * Chroot, revoke privileges, and pledge(2) the server.
 * This should be called even in debug mode.
 */
static void
privdrop(void)
{
	/* Sandbox in a chroot and drop privileges */
	if (debug == 0) {
		struct passwd *password;
		if (!(password = getpwnam(_PW_USER))) {
			err(1, "getpwnam failed");
		}
		if (chroot(_PW_DIR) == -1) {
			err(1, "chroot failed");
		}
		if (chdir("/") == -1) {
			err(1, "chdir failed");
		}
		if (setgroups(1, &password->pw_gid) == -1 ||
		    setresgid(password->pw_gid, password->pw_gid, password->pw_gid) == -1 ||
		    setresuid(password->pw_uid, password->pw_uid, password->pw_uid) == -1) {
			err(1, "privdrop failed");
		}
	}

	/* Restrict the server */
	if (pledge("stdio inet proc", NULL) == -1) {
		err(1, "pledge failed");
	}

	DPRINTF("server is sandboxed\n");

	/* Run in the background like a real system process */
	if (debug == 0) {
		if (daemon(1, 0) == -1) {
			err(1, "daemon failed");
		}
	}
}

int
main(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, "d")) != -1) {
		switch (c) {
			case 'd':
				debug = 1;
				break;
			default:
				usage();
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0) {
		usage();
	}

	if (debug == 0) {
		/* Need to be root to bind to low port */
		if (geteuid() != 0) {
			errx(1, "need root privileges");
		}
	}

	int port = (debug == 1) ? DEBUG_PORT : PORT;

	/* Set up the server socket */
	struct sockaddr_in serversock;
	memset(&serversock, 0, sizeof(serversock));
	serversock.sin_family = AF_INET;
	serversock.sin_port = htons(port);
	serversock.sin_addr.s_addr = (debug == 1) ? htonl(DEBUG_ADDR) : htonl(ADDR);

	int listensd;		/* listening socket descriptor */
	listensd = socket(AF_INET, SOCK_STREAM, 0);
	if (listensd == -1) {
		err(1, "socket failed");
	}
	if (bind(listensd, (struct sockaddr *)&serversock, sizeof(serversock)) == -1) {
		err(1, "bind failed");
	}
	if (listen(listensd, 5) == -1) {
		err(1, "listen failed");
	}

	DPRINTF("server up and listening for connections on port %d\n", port);

	privdrop();

	/* Ensure children can be made without leaving zombies when they die */
	struct sigaction sa;
	sa.sa_handler = kidhandler;
	sigemptyset(&sa.sa_mask);
	/* Allow syscalls (like accept(2)) to be restarted if they're interrupted by a SIGCHLD */
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		err(1, "sigaction failed");
	}

	/* Accept client connections and serve the current time */
	char timestr[256];		/* holds the time string sent to the client */
	struct sockaddr_in clientsock;
	for (;;) {
		int clientsocklen = sizeof(&clientsock);
		int clientsd = accept(listensd, (struct sockaddr *)&clientsock, &clientsocklen);
		if (clientsd == -1) {
			err(1, "accept failed");
		}
		DPRINTF("connection accepted\n");

		pid_t pid;
		switch ((pid = fork())) {
		case -1:
			err(1, "fork failed");
		case 0:
			/* child */
			DPRINTF("child is processing\n");
			/* Restrict the child */
			if (pledge("stdio", NULL) == -1) {
				err(1, "child pledge failed");
			}
			getthetime(timestr, sizeof(timestr));
			size_t tslen = strnlen(timestr, sizeof(timestr));

			ssize_t nsent = send(clientsd, timestr, tslen, 0);
			if (nsent == -1) {
				err(1, "send failed");
			}
			DPRINTF("sent %zd chars\n", nsent);

			close(clientsd);
			DPRINTF("child - closed client connection sd %d - exiting child\n", clientsd);
			exit(0);
		default:
			/* parent */
			close(clientsd);
			DPRINTF("parent - closed client connection sd %d\n", clientsd);
		}
	}
}
