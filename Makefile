SHELL = /bin/sh
CFLAGS = -O3 -m486
#GTKCFLAGS = `gtk-config --cflags` -I. -O -Wall
GTKCFLAGS = `gtk-config --cflags`
LDFLAGS=
CC=gcc
BINDIR=/usr/local/bin
MANDIR=/usr/local/man/man1
LIBS = `gtk-config --libs`
 
all: htmlenc findenc

findenc : findenc.c encyfuncs.c ency.h
	$(CC) $(GTKCFLAGS)  encyfuncs.c $(LIBS) $< -o $@

htmlenc : htmlenc.c encyfuncs.c ency.h
	$(CC) encyfuncs.c $< -o $@

clean :
	rm -f findenc htmlenc core

install: findenc
	install -c findenc $(BINDIR)
	install -c findenc.1 $(MANDIR)
	install -c htmlenc $(BINDIR)
	install -c htmlenc.1 $(MANDIR)
 

uninstall:
	rm -f $(BINDIR)/findenc $(MANDIR)/findenc.1 $(BINDIR)/htmlenc $(MANDIR)/htmlenc.1
