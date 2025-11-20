# Makefile for frotzl - Frotz launcher

CC = gcc
CFLAGS = -Wall -O2 -g
LDFLAGS =
PKG_CONFIG = pkg-config

# Get SDL flags
SDL_CFLAGS = $(shell $(PKG_CONFIG) sdl2 SDL2_ttf --cflags)
SDL_LDFLAGS = $(shell $(PKG_CONFIG) sdl2 SDL2_ttf --libs)

# Installation paths
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

# Target
TARGET = frotzl
SRCS = frotzl.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(SDL_LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

# Dependencies
frotzl.o: frotzl.c
