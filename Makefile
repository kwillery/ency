SHELL = /bin/sh
CFLAGS = -O3 -m486
#GTKCFLAGS = `gtk-config --cflags` -I. -O -Wall
GTKCFLAGS = `gtk-config --cflags`
LDFLAGS=
CC=gcc
DOCDIR=/usr/doc
BINDIR=/usr/bin
MANDIR=/usr/man
LIBS = `gtk-config --libs`

all: htmlenc findenc gtkenc

no-gtk: findenc htmlenc

findenc : findenc.c encyfuncs.c ency.h
	$(CC) encyfuncs.c $< -o $@

htmlenc : htmlenc.c encyfuncs.c ency.h
	$(CC) encyfuncs.c $< -o $@

gtkenc : gtkenc.c encyfuncs.c ency.h
	$(CC) $(GTKCFLAGS)  encyfuncs.c $(LIBS) $< -o $@

clean :
	rm -f findenc htmlenc gtkenc core

install: findenc htmlenc gtkenc
	install -c findenc $(BINDIR)
#	install -c findenc.1 $(MANDIR)
	install -c htmlenc $(BINDIR)
#	install -c htmlenc.1 $(MANDIR)
	install -c gtkenc $(BINDIR)

nogtk-install: findenc htmlenc
	install -c findenc $(BINDIR)
	install -c htmlenc $(BINDIR)

uninstall:
	rm -f $(BINDIR)/findenc $(MANDIR)/findenc.1 $(BINDIR)/htmlenc $(MANDIR)/htmlenc.1 $(BINDIR)/gtkenc

