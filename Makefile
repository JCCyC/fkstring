PREFIX=/usr/local

CFLAGS=-I. -Wall -fpic
LDFLAGS=-L. -Wall -fpic

all: libfkstring.a libfkstring.so smoketest

libfkstring.a: fkstring.o fkstrerr.o fkstdio.o
	ar rcs libfkstring.a fkstring.o fkstrerr.o fkstdio.o

libfkstring.so: fkstring.o fkstrerr.o fkstdio.o
	gcc -Wall -shared -o libfkstring.so fkstring.o fkstrerr.o fkstdio.o

smoketest: smoketest.c fkstdio.o fkstrerr.o fkstring.o
	gcc $(CFLAGS) -c -o smoketest.o smoketest.c
	gcc $(LDFLAGS) -static -o smoketest smoketest.o -lfkstring -lm

fkstring.o: fkstring.c fkstring.h fkstring_internal.h Makefile
	gcc $(CFLAGS) -c fkstring.c -o fkstring.o

fkstdio.o: fkstdio.c fkstring.h fkstring_internal.h Makefile
	gcc $(CFLAGS) -c fkstdio.c -o fkstdio.o

fkstrerr.o: fkstrerr.c fkstring.h fkstring_internal.h Makefile
	gcc $(CFLAGS) -c fkstrerr.c -o fkstrerr.o

clean:
	rm -f smoketest libfkstring.a *.o *.so
	$(MAKE) -C tests clean

check: libfkstring.a
	$(MAKE) -C tests
	./tests/alltests

test: check

install: libfkstring.a libfkstring.so
	install -o root -g root -m 755 libfkstring.a libfkstring.so $(PREFIX)/lib
	install -o root -g root -m 644 fkstring.h fkstring_internal.h $(PREFIX)/include
