SHELL = /bin/sh
CFLAGS = -O3 -m486
LDFLAGS=
CC=gcc
BINDIR=/usr/local/bin
MANDIR=/usr/local/man/man1
 
all: htmlenc findenc

findenc : findenc.c encyfuncs.c ency.h
	$(CC) $(CFLAGS) encyfuncs.c $< -o $@

htmlenc : htmlenc.c encyfuncs.c ency.h
	$(CC) $(CFLAGS) encyfuncs.c $< -o $@

clean :
	rm -f findenc htmlenc core

install: findenc
	install -c findenc $(BINDIR)
	install -c findenc.1 $(MANDIR)
	install -c htmlenc $(BINDIR)
	install -c htmlenc.1 $(MANDIR)
 

uninstall:
	rm -f $(BINDIR)/findenc $(MANDIR)/findenc.1 $(BINDIR)/htmlenc $(MANDIR)/htmlenc.1
