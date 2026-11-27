
CFLAGS=-w -Wall -g
ENGINE_CFLAGS=-Wall -Wextra -Werror -g
GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 gthread-2.0`
GTK_LDFLAGS = `pkg-config --libs gtk+-2.0 gthread-2.0`

all:	dmost

dmost:	dmost.c
	gcc -o dmost ${CFLAGS} ${GTK_CFLAGS} dmost.c ${GTK_LDFLAGS}

game.o:	game.c game.h
	gcc -c ${ENGINE_CFLAGS} game.c

test_game:	test_game.c game.o game.h
	gcc -o test_game ${ENGINE_CFLAGS} test_game.c game.o

.PHONY: test clean

test:	test_game
	./test_game

clean:
	rm -f dmost test_game game.o


