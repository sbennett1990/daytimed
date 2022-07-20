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

user:
	doas user add -c"daytime daemon" \
            -d/var/empty \
            -p* \
            -s/sbin/nologin \
            _daytimed

.include <bsd.prog.mk>
