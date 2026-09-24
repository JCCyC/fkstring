#ifndef __FKSTRING_H__
#define __FKSTRING_H__

#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct _fkstring
{
	size_t	len;		/* If len == 0, alloc and cstr MUST be 0 and NULL respectively */
	size_t	alloc;
	char	*cstr;
} fkstring;

#define fkstrlen(fks) ((fks)->len)
#define fkstrsize(fks) ((fks)->alloc)
#define fkcstr(fks) ((fks)->cstr)

/* Returned by the search functions (fkstrfind() etc.) when there's no match. */
#define FKSTR_NPOS ((size_t)-1)

/* fkstring.c */
fkstring *fkstrnew(const char *s);
fkstring *fkstrnewb(const void *buf, size_t len);
fkstring *fkalloc(size_t size);
fkstring *fkcalloc(size_t nmemb, size_t size);
void fkstrdestroy(fkstring *fks);
fkstring *fkstrdup(const fkstring *fks);
fkstring *fkstrcat(fkstring *dst, const fkstring *src);
fkstring *fkstrcatc(fkstring *dst, const char *src);
fkstring *fkstrcatone(fkstring *dst, char c);
fkstring *fkinsert(fkstring *fks, size_t pos, const fkstring *src);
fkstring *fkinsertc(fkstring *fks, size_t pos, const char *cstr);
fkstring *fksubstr(const fkstring *fstr, size_t start, size_t len);
fkstring *fkstrtrunc(fkstring *fks, size_t newlen);
fkstring *fkslack(fkstring *fks, size_t n);
fkstring *fkfit(fkstring *fks);
size_t fkremove(fkstring *fstr, size_t start, size_t len);
size_t fkltrim(fkstring *fks);
size_t fkrtrim(fkstring *fks);
size_t fktrim(fkstring *fks);
fkstring **fksplit(const fkstring *src, char delim);
void fkarraydestroy(fkstring **fka);
fkstring *fkjoin(fkstring **arr, const char *sep);
int fkstrcmp(const fkstring *a, const fkstring *b);
int fkstrcasecmp(const fkstring *a, const fkstring *b);
int fkstreq(const fkstring *a, const fkstring *b);
size_t fkstrfind(const fkstring *hay, const fkstring *needle, size_t start);
size_t fkstrfindc(const fkstring *hay, const char *needle, size_t start);
size_t fkstrchr(const fkstring *fks, char c, size_t start);
size_t fkstrrchr(const fkstring *fks, char c);
int fkstartswith(const fkstring *fks, const fkstring *prefix);
int fkstartswithc(const fkstring *fks, const char *prefix);
int fkendswith(const fkstring *fks, const fkstring *suffix);
int fkendswithc(const fkstring *fks, const char *suffix);
fkstring *fkreplace(fkstring *fks, const fkstring *old, const fkstring *new, size_t max_count);
fkstring *fkreplacec(fkstring *fks, const char *old, const char *new, size_t max_count);

/* fkstdio.c */
fkstring *fksprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
fkstring *fkvsprintf(const char *fmt, va_list ap) __attribute__((format(printf, 1, 0)));
fkstring *fkstrcatf(fkstring *dst, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
fkstring *fkstrcatvf(fkstring *dst, const char *fmt, va_list ap) __attribute__((format(printf, 2, 0)));
ssize_t fkstrwrite(int fd, const fkstring *fks);
fkstring *fkstrread(int fd, size_t count);
fkstring *fkreadline(FILE *fp);

#endif /* __FKSTRING_H__ */
