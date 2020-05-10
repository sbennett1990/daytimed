
PROG=	daytimed
SRCS=	daytimed.c

MAN=	daytimed.8
README=	README.md

CFLAGS+= -Wall
CFLAGS+= -Wstrict-prototypes -Wmissing-prototypes
CFLAGS+= -Wmissing-declarations
CFLAGS+= -Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+= -Wsign-compare

md:
	mandoc -Tmarkdown ${MAN} > ${README}

.include <bsd.prog.mk>
