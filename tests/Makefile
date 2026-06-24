CFLAGS=-I.. -Wall -fpic
LDFLAGS=-L.. -Wall -fpic

SRCS=framework.c \
     test_allocforlen.c \
     test_fkpanic.c \
     test_fkstrerr.c \
     test_fkstrnew.c \
     test_fkstrdestroy.c \
     test_fkstrdup.c \
     test_fkstrtrunc.c \
     test_fkstrcat.c \
     test_fksubstr.c \
     test_fkremove.c \
     test_fktrim.c \
     test_fksprintf.c \
     test_fkstrwrite.c \
     test_fkstrread.c \
     alltests.c
OBJS=$(SRCS:.c=.o)

alltests: $(OBJS) ../libfkstring.a
	gcc $(LDFLAGS) -static -o alltests $(OBJS) -lfkstring -lm

%.o: %.c framework.h ../fkstring.h ../fkstring_internal.h Makefile
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f alltests *.o
