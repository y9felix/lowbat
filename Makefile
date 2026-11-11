PROG = lowbat
VERSION = 1.0
PREFIX = /usr/local

CC = cc
WAYLAND_SCANNER = wayland-scanner

PKGS = wayland-client

CPPFLAGS = -DVERSION=\"$(VERSION)\" $$(pkg-config --cflags $(PKGS))
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Werror -O2
LDFLAGS = -s $$(pkg-config --libs $(PKGS))

PROTO_XML = protocols/wlr-layer-shell-unstable-v1.xml
PROTO_H = wlr-layer-shell-unstable-v1-client-protocol.h
PROTO_C = wlr-layer-shell-unstable-v1-protocol.c

SRC = $(PROG).c $(PROTO_C)
OBJ = $(SRC:.c=.o)

all: $(PROG)

$(OBJ): config.h $(PROTO_H)

config.h:
	cp config.def.h $@
	chmod 644 $@

$(PROTO_H): $(PROTO_XML)
	$(WAYLAND_SCANNER) client-header $< $@
	chmod 644 $@

$(PROTO_C): $(PROTO_XML)
	$(WAYLAND_SCANNER) private-code $< $@
	chmod 644 $@

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<
	chmod 644 $@

$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
	chmod 755 $@

clean:
	rm -f $(PROG) $(OBJ) $(PROTO_H) $(PROTO_C)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROG)
	cp -f $(PROG) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(PROG)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(PROG)

.PHONY: all clean install uninstall
