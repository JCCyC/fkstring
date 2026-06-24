#include <stdio.h>
#include <time.h>
#include <math.h>
#include "fkstring.h"

void fkshow(fkstring *fks)
{
	// printf("Len = %3d, Alloc = %3d, Str = '%s'\n", fkstrlen(fks), fkstrsize(fks), fkcstr(fks));
	if (!fks)
		printf("Null fkstr\n");
	else
	{
		printf("Len = %3zu, Alloc = %3zu, Str = '", fkstrlen(fks), fkstrsize(fks));
		fflush(stdout);
		fkstrwrite(1, fks);
		write(1, "'\n", 2);
	}
}

void testremove(fkstring *fstr, int start, int len)
{
	int retval = fkremove(fstr, start, len);
	printf("Removing %3d, %3d ret = %3d: ", start, len, retval);
	fkshow(fstr);
}

int main(int argc, char **argv)
{
	int	i;

	fkstring *test1 = fkstrnew("Jacare no seco anda");
	fkstring *test2 = fksubstr(test1, 7, 7);
	fkstring *test3;
	fkstring *excl = fkstrnew("!!");
	fkstring *fkstdin = fkstrread(0, 32767);
	fkstring *fkbadfd = fkstrread(849237, 32767);

	printf("From std in: "); fkshow(fkstdin);
	printf("From bad fd: "); fkshow(fkbadfd);

	fkshow(test1);
	fkshow(test2);

	fkstrcatc(test2, " e no molhado");
	fkshow(test2);

	for (i = 0; i < 60; i++)
	{
		fkstrcat(test2, excl);
		fkshow(test2);
	}
	fkstrdestroy(excl);

	for (i = fkstrlen(test2); i >= 0; i -= 10)
	{
		if (fkstrtrunc(test2, i))
			fkshow(test2);
		else
			printf("fkstrtrunc(test2, %d) failed\n", i);
	}

	test3 = fksprintf("T=%lu", time(NULL));
	fkshow(test3);
	fkstrdestroy(test3);

	i = 3;
	char *sample="Sample";
	double pi = 3.14159265358979323846;
	test3 = fksprintf("Powers and String and Sine: %d, %d, %s, %d, %d, %lf", i, i*i, sample, i*i*i, i*i*i*i, sin(pi/4));
	fkshow(test3);
	fkstrdestroy(test3);

	test3 = fksprintf("%d args, progname = [%s]", argc, argv[0]);
	fkshow(test3);
	fkstrdestroy(test3);

	printf("Via C: argc = %d, argv[0] = [%s]\n", argc, argv[0]);

	fkstrdestroy(test2);
	fkstrdestroy(test1);

	test1 = fkstrnew("The Quick Brown Fox Jumped Over the Lazy Dog");
	fkshow(test1);
	testremove(test1, 96, 57);
	testremove(test1, 40, 57);
	testremove(test1, 38, 1);
	testremove(test1, 4, 6);
	testremove(test1, 3, 30);
	testremove(test1, 0, 3);

	fkstrdestroy(test1);
	return 0;
}
