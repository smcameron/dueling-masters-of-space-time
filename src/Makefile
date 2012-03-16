
CFLAGS=-w -Wall -g
GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 gthread-2.0`
GTK_LDFLAGS = `pkg-config --libs gtk+-2.0 gthread-2.0`

dmost:	dmost.c
	gcc -o dmost ${CFLAGS} ${GTK_CFLAGS} dmost.c ${GTK_LDFLAGS}

