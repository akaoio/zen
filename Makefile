exec = zen
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g


$(exec): $(objects)
	gcc $(objects) $(flags) -o $(exec)

%.o: %.c include/%.h
	gcc -c $(flags) $< -o $@

install:
	make
	cp ./zen $(PREFIX)/bin/zen

clean:
	-rm -f *.out
	-rm -f *.o
	-rm -f src/*.o
	-rm -f zen
