CFLAGS = -O2 -g -Wall -Wextra 
LDFLAGS = -ldel -levent -luuid -lsqlite3 -ldsalloc
objects = basic.o

basic: $(objects)
	cc $(CFLAGS) $(objects) -o basic $(LDFLAGS)

clean:
	rm -f basic *.o
	rm -rf *.dSYM

