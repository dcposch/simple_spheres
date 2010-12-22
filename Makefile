CFILES= demo.c geometry.c
CFLAGS= -Wall -lSDL -lrt -std=gnu99
HASKELLFILES= demo_monad.hs
HASKELLFLAGS= --make -O2

all: $(CFILES)
	gcc -O3 $(CFLAGS) $(CFILES) -o demo

debug: $(CFILES)
	gcc -g -O0 $(CFLAGS) $(CFILES) -o demo

profile: $(CFILES)
	gcc -pg -O3 $(CFLAGS) $(CFILES) -o demo 

monad: $(HASKELLFILES) 
	ghc $(HASKELLFLAGS) $(HASKELLFILES)

clean:
	rm -f demo demo_monad.hi demo_monad.o demo_monad
