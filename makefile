GPP=g++
FLAGS=-Wall -Werror -lpthread --std=c++17
DFLAGS=-g -Wall -Werror -lpthread --std=c++17


build:
	$(GPP) -o pipeline pipeline.cpp $(FLAGS)
debug:
	$(GPP) -o pipeline pipeline.cpp $(DFLAGS)

.PHONY: clean
clean:
	rm -f pipeline *.out
 