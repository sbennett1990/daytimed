
PROG=	daytimed
SRCS=	daytimed.c

MAN=	daytimed.8

CFLAGS+= -Wall
CFLAGS+= -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= -Wmissing-declarations
CFLAGS+= -Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+= -Wsign-compare

md:
	mandoc -Tmarkdown ${MAN} > ${PROG}.md

.include <bsd.prog.mk>
