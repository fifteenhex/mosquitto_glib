PKGCONFIG ?= pkg-config
CFLAGS = --std=gnu99 -ggdb
GLIB = `$(PKGCONFIG) --cflags --libs glib-2.0 gio-2.0`

mosquittomainloop.a: mosquittomainloop.c mosquittomainloop.h
	$(CC) $(CFLAGS) $(GLIB) -c mosquittomainloop.c 