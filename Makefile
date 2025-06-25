CFLAGS = -O2 -g -Wall -Wextra
objects = dbi.o

dbi: $(objects) cli.o
	$(CC) $(CFLAGS) $(objects) cli.o -o dbi

dbi_sandboxed.o: dbi.c dbi.h
	$(CC) $(CFLAGS) -DDBI_DISABLE_IO=1 -c dbi.c -o dbi_sandboxed.o

libdbi.a: dbi_sandboxed.o
	ar rc libdbi.a dbi_sandboxed.o

install: dbi libdbi.a
	@sudo mkdir -p /usr/local/lib
	@sudo mkdir -p /usr/local/include
	@sudo cp libdbi.a /usr/local/lib && echo "libdbi.a installed at /usr/local/lib"
	@sudo cp dbi.h /usr/local/include && echo "dbi.h installed at /usr/local/include"
	@sudo mkdir -p /usr/local/bin
	@sudo cp dbi /usr/local/bin && echo "dbi installed at /usr/local/bin/dbi"

uninstall:
	@sudo rm -f /usr/local/lib/libdbi.a
	@sudo rm -f /usr/local/lib/libdbi.so
	@sudo rm -f /usr/local/bin/dbi
	@sudo rm -f /usr/local/include/dbi.h

example: libdbi.a example.o dbi.h
	@make install
	$(CC) $(CFLAGS) $(objects) example.o -o example

test: dbi
	@./dbi tests/test.bas

clean:
	rm -f dbi *.o *.a *.so
	rm -rf *.dSYM

