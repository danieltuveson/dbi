CC=gcc
CFLAGS = -O2 -g -Wall -Wextra 
objects = dbi.o

dbi: $(objects)
	$(CC) $(CFLAGS) $(objects) -o dbi

pres:
	$(CC) $(CFLAGS) pres.c -o pres

clean:
	rm -f dbi pres *.o
	rm -rf *.dSYM

