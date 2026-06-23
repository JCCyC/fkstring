#ifndef __FKSTRING_H__
#define __FKSTRING_H__

#include <unistd.h>
#include <string.h>

typedef struct _fkstring
{
	int	len;		/* If len == 0, alloc and cstr MUST be 0 and NULL respectively */
	int	alloc;
	char	*cstr;
} fkstring;

#define fkstrlen(fks) ((fks)->len)
#define fkstrsize(fks) ((fks)->alloc)
#define fkcstr(fks) ((fks)->cstr)

/* fkstring.c */
fkstring *fkstrnew(const char *s);
void fkstrdestroy(fkstring *fks);
fkstring *fkstrdup(fkstring *fks);
fkstring *fkstrcat(fkstring *dst, fkstring *src);
fkstring *fkstrcatc(fkstring *dst, char *src);
fkstring *fkstrcatone(fkstring *dst, char c);
fkstring *fksubstr(fkstring *fstr, int start, int len);
fkstring *fkstrtrunc(fkstring *fks, int newlen);
int fkremove(fkstring *fstr, int start, int len);

/* fkstdio.c */
fkstring *fksprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
ssize_t fkstrwrite(int fd, fkstring *fks);
fkstring *fkstrread(int fd, size_t count);

#endif /* __FKSTRING_H__ */
