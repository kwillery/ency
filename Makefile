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

all: libency.a(encyfuncs.o) htmlenc findenc showm

libency.a: libency.a(encyfuncs.o)

libency.a(encyfuncs.o): ency.h encyfuncs.c

findenc htmlenc showm: libency.a

clean :
	rm -f findenc htmlenc showm encyfuncs.o libency.a core

install: findenc htmlenc showm
	install -c findenc $(BINDIR)
	install -c htmlenc $(BINDIR)
	install -c showm $(BINDIR)
	install -c libency.a $(LIBDIR)
	install -c ency.h $(INCDIR)

uninstall:
	rm -f $(BINDIR)/findenc $(BINDIR)/htmlenc $(BINDIR)/showm $(LIBDIR)/libency.a $(INCDIR)/ency.h

doc: ency-api.txt ency-api.html

ency-api.txt: ency-api.sgml
	sgml2txt ency-api.sgml
ency-api.html: ency-api.sgml
	sgml2html ency-api.sgml
