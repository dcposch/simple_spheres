FILES= demo.c geometry.c
CFLAGS= -Wall -lSDL

all: $(FILES)
	gcc -O3 $(CFLAGS) $(FILES) -o demo

debug: $(FILES)
	gcc -g -O0 $(CFLAGS) $(FILES) -o demo

profile: $(FILES)
	gcc -pg -O3 $(CFLAGS) $(FILES) -o demo 

clean:
	rm -f demo
