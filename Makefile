SHELL = /bin/sh
CFLAGS = -O3 -g
LDFLAGS=
CC=gcc
AR=ar
DOCDIR=/usr/doc
BINDIR=/usr/bin
MANDIR=/usr/man
LIBDIR=/usr/lib
INCDIR=/usr/include

.PHONY: all clean install uninstall
all: libency.a(encyfuncs.o) htmlenc findenc

libency.a(encyfuncs.o): ency.h

findenc htmlenc: libency.a

clean :
	rm -f findenc htmlenc encyfuncs.o libency.a core

install: findenc htmlenc
	install -c findenc $(BINDIR)
	install -c htmlenc $(BINDIR)
	install -c libency.a $(LIBDIR)
	install -c ency.h $(INCDIR)

uninstall:
	rm -f $(BINDIR)/findenc $(BINDIR)/htmlenc $(LIBDIR)/libency.a $(INCDIR)/ency.h

