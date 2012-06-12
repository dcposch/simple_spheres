# DC
# Spring 2012

CFILES= demo.c geometry.c
SDL_FLAGS= $(shell sdl-config --cflags) $(shell sdl-config --libs) 
CFLAGS= -Wall -std=c99 $(SDL_FLAGS)

all: $(CFILES)
	gcc -O3 $(CFILES) $(CFLAGS) -o demo

debug: $(CFILES)
	gcc -g -O0 $(CFILES) $(CFLAGS) -o demo

profile: $(CFILES)
	gcc -pg -O3 $(CFILES) $(CFLAGS) -o demo 

clean:
	rm -f demo 
