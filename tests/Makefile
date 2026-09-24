CFLAGS=-I.. -Wall -fpic
LDFLAGS=-L.. -Wall -fpic

SRCS=framework.c \
     test_allocforlen.c \
     test_fkaddlen.c \
     test_fkmullen.c \
     test_fkpanic.c \
     test_fkstrerr.c \
     test_fkstrnew.c \
     test_fkalloc.c \
     test_fkstrdestroy.c \
     test_fkstrdup.c \
     test_fkstrtrunc.c \
     test_fkstrcat.c \
     test_fkstrcatf.c \
     test_fkinsert.c \
     test_fksubstr.c \
     test_fkremove.c \
     test_fktrim.c \
     test_fksplit.c \
     test_fkjoin.c \
     test_fkstrcmp.c \
     test_fkstrfind.c \
     test_fkstrchr.c \
     test_fkstartswith.c \
     test_fkreplace.c \
     test_fksprintf.c \
     test_fkstrwrite.c \
     test_fkstrread.c \
     test_fkreadline.c \
     alltests.c
OBJS=$(SRCS:.c=.o)

alltests: $(OBJS) ../libfkstring.a
	gcc $(LDFLAGS) -static -o alltests $(OBJS) -lfkstring -lm

# Dynamically linked against libc so Valgrind can intercept malloc()/free();
# it can't track the heap of the -static alltests binary.
alltests-memcheck: $(OBJS) ../libfkstring.a
	gcc $(LDFLAGS) -o alltests-memcheck $(OBJS) ../libfkstring.a -lm

%.o: %.c framework.h ../fkstring.h ../fkstring_internal.h Makefile
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f alltests alltests-memcheck *.o
