SHELL = /bin/sh
CFLAGS = -O3
LDFLAGS=
CC=gcc
AR=ar
DOCDIR=/usr/doc
BINDIR=/usr/bin
MANDIR=/usr/man
LIBDIR=/usr/lib
INCDIR=/usr/include

all: libency htmlenc findenc

libency : encyfuncs.c ency.h
	$(CC) -o libency.o -c encyfuncs.c
	$(AR) r libency.a libency.o

findenc : findenc.c ency.h libency
	$(CC) $< -o $@ -L. -lency

htmlenc : htmlenc.c ency.h libency
	$(CC) $< -o $@ -L. -lency

clean :
	rm -f findenc htmlenc libency.o libency.a core

install: findenc htmlenc
	install -c findenc $(BINDIR)
	install -c htmlenc $(BINDIR)
	install -c libency.a $(LIBDIR)
	install -c ency.h $(INCDIR)

uninstall:
	rm -f $(BINDIR)/findenc $(BINDIR)/htmlenc

