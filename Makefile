CFLAGS=-O3 -g -Wall
LDFLAGS=
CC=gcc
AR=ar
prefix=/usr/local
DOCDIR=$(prefix)/doc
BINDIR=$(prefix)/bin
MANDIR=$(prefix)/man
LIBDIR=$(prefix)/lib
INCDIR=$(prefix)/include
DATADIR=$(prefix)/share/ency

INSTALL_PROGRAM=install

ifneq (,$(findstring debug,$(DEB_BUILD_OPTIONS)))
  CFLAGS += -g
endif
ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTIONS)))
  INSTALL_BINOPTS += -s
endif
INSTALL_BIN=$(INSTALL_PROGRAM) $(INSTALL_BINOPTS)

.PHONY: all clean distclean install uninstall

all: libency.a htmlenc findenc scanenc

libency.a: libency.a(encyfuncs.o) libency.a(data.o) libency.a(scan.o) libency.a(rcfile.o) libency.a(pictures.o) libency.a(esdata.o)

libency.a(encyfuncs.o): ency.h encyfuncs.c data.h

libency.a(data.o): data.h data.c ency.h

libency.a(scan.o): scan.c scan.h

libency.a(rcfile.o): rcfile.c rcfile.h

libency.a(pictures.o): pictures.c pictures.h

libency.a(esdata.o): esdata.c esdata.h

findenc htmlenc scanenc: libency.a

clean:
	rm -f findenc htmlenc scanenc encyfuncs.o data.o scan.o rcfile.o pictures.o esdata.o libency.a core

distclean: clean

install: install-bin install-data install-dev install-doc

install-bin: all
	$(INSTALL_BIN) findenc $(BINDIR)/findenc
	$(INSTALL_BIN) htmlenc $(BINDIR)/htmlenc
	$(INSTALL_BIN) scanenc $(BINDIR)/scanenc

install-data:
	$(INSTALL_PROGRAM) encyfiles.rc $(DATADIR)/encyfiles.rc

install-doc:

install-dev:
	$(INSTALL_BIN) -m644 libency.a $(LIBDIR)/libency.a
	$(INSTALL_PROGRAM) -m644 ency.h $(INCDIR)/ency.h

uninstall:
	rm -f $(BINDIR)/findenc $(BINDIR)/htmlenc $(LIBDIR)/libency.a $(INCDIR)/ency.h $(BINDIR)/scanenc $(DATADIR)/encyfiles.rc

doc: ency-api.txt ency-api.html

ency-api.txt: ency-api.sgml
	sgml2txt ency-api.sgml
ency-api.html: ency-api.sgml
	sgml2html ency-api.sgml

gtkscan: gtkscan.c libency.a
	$(CC) $(CFLAGS) -o gtkscan gtkscan.c libency.a `gtk-config --cflags` `gtk-config --libs`
