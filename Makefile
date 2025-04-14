CC=gcc
CFLAGS = -O2 -g -Wall -Wextra
objects = dbi.o

dbi: $(objects)
	$(CC) $(CFLAGS) $(objects) -o dbi

clean:
	rm -f dbi *.o
	rm -rf *.dSYM

