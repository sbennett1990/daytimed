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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int debug = 0;
char *timeformat = "%a %b %e %H:%M:%S %Z %Y";

#define PORT	17017

#define DPRINTF(x...) do { if (debug) printf(x); } while (0)

void
usage()
{
	printf("datetimed [-d]\n");
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

	struct tm *tp = localtime(&tval);
	if (tp == NULL) {
		err(1, "time conversion error");
	}

	strftime(buf, len, timeformat, tp);
}

int
main(int argc, char **argv)
{
	char timestr[1024];

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

	if (argc > 1) {
		usage();
		exit(1);
	}

	int port = PORT;
	int sd;		/* socket descriptor */
	struct sockaddr_in sockname, client;

	/* set up the socket */
	memset(&sockname, 0, sizeof(sockname));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(port);
	sockname.sin_addr.s_addr = htonl(INADDR_ANY);

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1)
		err(1, "socket failed");
	if (bind(sd, (struct sockaddr *)&sockname, sizeof(sockname)) == -1)
		err(1, "bind failed");
	if (listen(sd, 3) == -1)
		err(1, "listen failed");

	DPRINTF("server up and listening for connections on port %d\n", port);

	for (;;) {
		int clientlen = sizeof(&client);
		int clientsd = accept(sd, (struct sockaddr *)&client, &clientlen);
		if (clientsd == -1)
			err(1, "accept failed");

		getthetime(timestr, sizeof(timestr));
		printf("%s\n", timestr);
		
		close(clientsd);
	}
}
