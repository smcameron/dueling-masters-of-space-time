
CFLAGS=-Wall -Wextra -g
ENGINE_CFLAGS=-Wall -Wextra -Werror -g
SDL_CFLAGS = `pkg-config --cflags sdl2 libpng`
SDL_LDFLAGS = `pkg-config --libs sdl2 libpng`

all:	dmost

dmost:	dmost.c game.o ai.o sdl_font.o sdl_gfx.o sdl_ui.o game.h ai.h opponent.h sdl_font.h sdl_gfx.h sdl_ui.h
	gcc -o dmost ${CFLAGS} ${SDL_CFLAGS} dmost.c game.o ai.o sdl_font.o sdl_gfx.o sdl_ui.o ${SDL_LDFLAGS} -lm

game.o:	game.c game.h
	gcc -c ${ENGINE_CFLAGS} game.c

ai.o:	ai.c ai.h opponent.h game.h
	gcc -c ${ENGINE_CFLAGS} ai.c

sdl_font.o:	sdl_font.c sdl_font.h
	gcc -c ${ENGINE_CFLAGS} ${SDL_CFLAGS} sdl_font.c

sdl_gfx.o:	sdl_gfx.c sdl_gfx.h
	gcc -c ${ENGINE_CFLAGS} ${SDL_CFLAGS} sdl_gfx.c

sdl_ui.o:	sdl_ui.c sdl_ui.h sdl_font.h sdl_gfx.h
	gcc -c ${ENGINE_CFLAGS} ${SDL_CFLAGS} sdl_ui.c

test_game:	test_game.c game.o game.h
	gcc -o test_game ${ENGINE_CFLAGS} test_game.c game.o

test_ai:	test_ai.c ai.o game.o ai.h opponent.h game.h
	gcc -o test_ai ${ENGINE_CFLAGS} test_ai.c ai.o game.o

.PHONY: test clean

test:	test_game test_ai
	./test_game
	./test_ai

clean:
	rm -f dmost test_game test_ai game.o ai.o sdl_font.o sdl_gfx.o sdl_ui.o


