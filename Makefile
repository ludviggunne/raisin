CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Werror -g $(shell curl-config --cflags) -Ilib/ -Ilib/thpool/
LDFLAGS=-lexpat -lnewt $(shell curl-config --libs) -lc

PREFIX?=/usr/local
DESTDIR?=
BINDIR=$(DESTDIR)$(PREFIX)/bin/

SRC=$(wildcard *.c) lib/thpool/thpool.c
OBJ=$(SRC:%.c=%.o)

PROG=raisin

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

install:
	install -Dm775 $(PROG) $(BINDIR)$(PROG)

uninstall:
	rm $(BINDIR)$(PROG)

clean:
	rm -rf $(OBJ) $(PROG) **/*.o

.PHONY: all clean install
