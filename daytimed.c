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
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int debug = 0;
static const char *timeformat = "%a %b %e %H:%M:%S %Z %Y\n";

#define ADDR		INADDR_ANY
#define DEBUG_ADDR	INADDR_LOOPBACK
#define PORT		13013
#define _PW_USER	"_identd"
#define _PW_DIR		"/var/empty"

#define DPRINTF(x...) do { if (debug) printf(x); } while (0)

static void
usage()
{
	printf("daytimed [-d]\n");
	exit(1);
}

/*
 * Get the current date and time and write a human-readable string to buf.
 */
void
getthetime(char *buf, size_t len)
{
	time_t tval;
	if (time(&tval) == -1) {
		err(1, "time failed");
	}

	struct tm *timeptr = localtime(&tval);
	if (timeptr == NULL) {
		err(1, "time conversion error");
	}

	strftime(buf, len, timeformat, timeptr);
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

	/* Sandbox in a chroot and drop priveleges */
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
		if (setresgid(password->pw_gid, password->pw_gid, password->pw_gid) == -1 ||
		    setresuid(password->pw_uid, password->pw_uid, password->pw_uid) == -1) {
			err(1, "privdrop failed");
		}
	}

	/* Restrict the daemon */
	if (pledge("stdio inet proc", NULL) == -1) {
		err(1, "pledge failed");
	}

	/* Run in the background like a real system process */
	if (debug == 0) {
		if (daemon(1, 0) == -1) {
			err(1, "daemon failed");
		}
	}

	int port = PORT;
	int sd;		/* socket descriptor */
	struct sockaddr_in sockname, client;

	/* Set up the socket */
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = (debug == 1) ? htonl(DEBUG_ADDR) : htonl(ADDR);

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
		err(1, "socket failed");
	if (bind(sd, (struct sockaddr *)&sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");
	if (listen(sd, 3) == -1)
		err(1, "listen failed");

	DPRINTF("server up and listening for connections on port %d\n", port);

	char timestr[1024];
	for (;;) {
		int clientlen = sizeof(&client);
		int clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");
		DPRINTF("connection accepted\n");

		getthetime(timestr, sizeof(timestr));
		size_t tslen = strnlen(timestr, sizeof(timestr));

		ssize_t nsent = send(clientsd, timestr, tslen, 0);
		if (nsent == -1) {
			err(1, "send failed");
		}
		DPRINTF("sent %zd chars\n", nsent);

		close(clientsd);
	}
}
