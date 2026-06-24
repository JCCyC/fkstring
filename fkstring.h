#ifndef __FKSTRING_H__
#define __FKSTRING_H__

#include <unistd.h>
#include <string.h>
#include <stddef.h>

typedef struct _fkstring
{
	size_t	len;		/* If len == 0, alloc and cstr MUST be 0 and NULL respectively */
	size_t	alloc;
	char	*cstr;
} fkstring;

#define fkstrlen(fks) ((fks)->len)
#define fkstrsize(fks) ((fks)->alloc)
#define fkcstr(fks) ((fks)->cstr)

/* fkstring.c */
fkstring *fkstrnew(const char *s);
fkstring *fkstrnewb(const void *buf, size_t len);
void fkstrdestroy(fkstring *fks);
fkstring *fkstrdup(const fkstring *fks);
fkstring *fkstrcat(fkstring *dst, const fkstring *src);
fkstring *fkstrcatc(fkstring *dst, const char *src);
fkstring *fkstrcatone(fkstring *dst, char c);
fkstring *fksubstr(const fkstring *fstr, size_t start, size_t len);
fkstring *fkstrtrunc(fkstring *fks, size_t newlen);
size_t fkremove(fkstring *fstr, size_t start, size_t len);
size_t fkltrim(fkstring *fks);
size_t fkrtrim(fkstring *fks);
size_t fktrim(fkstring *fks);

/* fkstdio.c */
fkstring *fksprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
ssize_t fkstrwrite(int fd, const fkstring *fks);
fkstring *fkstrread(int fd, size_t count);

#endif /* __FKSTRING_H__ */
