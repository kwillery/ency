SHELL = /bin/sh
CFLAGS = -O3 -g -Wall
LDFLAGS=
CC=gcc
AR=ar
DOCDIR=/usr/doc
BINDIR=/usr/bin
MANDIR=/usr/man
LIBDIR=/usr/lib
INCDIR=/usr/include

.PHONY: all clean install uninstall

all: libency.a(encyfuncs.o) htmlenc findenc scanenc

libency.a: libency.a(encyfuncs.o)

libency.a(encyfuncs.o): ency.h encyfuncs.c

findenc htmlenc scanenc: libency.a

clean:
	rm -f findenc htmlenc encyfuncs.o libency.a core

install: findenc htmlenc
	install -c findenc $(BINDIR)
	install -c htmlenc $(BINDIR)
	install -c scanenc $(BINDIR)
	install -c libency.a $(LIBDIR)
	install -c ency.h $(INCDIR)

uninstall:
	rm -f $(BINDIR)/findenc $(BINDIR)/htmlenc $(LIBDIR)/libency.a $(INCDIR)/ency.h $(BINDIR)/scanenc

doc: ency-api.txt ency-api.html

ency-api.txt: ency-api.sgml
	sgml2txt ency-api.sgml
ency-api.html: ency-api.sgml
	sgml2html ency-api.sgml
