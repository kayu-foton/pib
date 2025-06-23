CC = gcc
CFLAGS = -Wall -Werror -pedantic -O3 -g3
LD_FLAGS = -lSDL2 -lSDL2_image -lm

.PHONY: run navmap
all: navmap run

navmap: navmap.c
	$(CC) -o navmap navmap.c $(CFLAGS) $(LD_FLAGS)

run: navmap
	./navmap
