#ifndef _GNU_SOURCE
#define _GNU_SOURCE	/* for memmem() on glibc/musl */
#endif
#include <unistd.h>
#include <stdlib.h>
#include <fkstring.h>
#include <stdio.h>
#include <fkstring_internal.h>

/* memmem() is a GNU/BSD extension, only standardized in POSIX.1-2024. Build
 * with -DFKSTR_HAVE_MEMMEM=0 (or =1) to override this detection. */
#ifndef FKSTR_HAVE_MEMMEM
#if defined(__GLIBC__) || defined(__FreeBSD__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__) || defined(__APPLE__) || \
    (defined(_POSIX_VERSION) && _POSIX_VERSION >= 202405L)
#define FKSTR_HAVE_MEMMEM 1
#else
#define FKSTR_HAVE_MEMMEM 0
#endif
#endif

int _bumpfactor = FKSTR_DEFAULT_BUMPFACTOR;
int _deflatefactor = FKSTR_DEFAULT_DEFLATEFACTOR;
size_t _minalloc = FKSTR_DEFAULT_MIN_ALLOC;
size_t _sprintftry = FKSTR_DEFAULT_SPRINTF_TRY;
size_t _slurptry = FKSTR_DEFAULT_SLURP_TRY;
size_t _catbufsize = FKSTR_DEFAULT_CAT_BUFSIZE;

void fkstrdestroy(fkstring *fks)
{
	if (fks)
	{
		if (fks->cstr)
			free(fks->cstr);
		free(fks);
	}
}

fkstring *fkstrnew(const char *s)
{
	size_t		newlen, newalloc;
	fkstring	*newfkstr;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	if (s && s[0])
	{
		newlen = strlen(s);
		newalloc = allocforlen(newlen);

		newfkstr->cstr = malloc(newalloc);
		if (!newfkstr->cstr)
			fkpanic(FKSTRERR_MEMALLOC);

		memcpy(newfkstr->cstr, s, newlen + 1);
		newfkstr->len = newlen;
		newfkstr->alloc = newalloc;
	}
	else
	{
		newfkstr->len = 0;
		newfkstr->alloc = 0;
		newfkstr->cstr = NULL;
	}

	return newfkstr;
}

fkstring *fkstrnewb(const void *buf, size_t len)
{
	size_t		newalloc;
	fkstring	*newfkstr;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	if (buf && len)
	{
		newalloc = allocforlen(len);

		newfkstr->cstr = malloc(newalloc);
		if (!newfkstr->cstr)
			fkpanic(FKSTRERR_MEMALLOC);

		memcpy(newfkstr->cstr, buf, len);
		newfkstr->cstr[len] = '\0';
		newfkstr->len = len;
		newfkstr->alloc = newalloc;
	}
	else
	{
		newfkstr->len = 0;
		newfkstr->alloc = 0;
		newfkstr->cstr = NULL;
	}

	return newfkstr;
}

/* Common to fkalloc()/fkcalloc(): a len-byte fkstring, zeroed on request. */
static fkstring *fkalloc_internal(size_t len, int zero)
{
	size_t		newalloc;
	fkstring	*newfkstr;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	if (len)
	{
		newalloc = allocforlen(len);

		newfkstr->cstr = zero ? calloc(newalloc, 1) : malloc(newalloc);
		if (!newfkstr->cstr)
			fkpanic(FKSTRERR_MEMALLOC);

		newfkstr->cstr[len] = '\0';
		newfkstr->len = len;
		newfkstr->alloc = newalloc;
	}
	else
	{
		newfkstr->len = 0;
		newfkstr->alloc = 0;
		newfkstr->cstr = NULL;
	}

	return newfkstr;
}

fkstring *fkalloc(size_t size)
{
	return fkalloc_internal(size, 0);
}

fkstring *fkcalloc(size_t nmemb, size_t size)
{
	return fkalloc_internal(fkmullen(nmemb, size), 1);
}

fkstring *fkstrtrunc(fkstring *fks, size_t newlen)
{
	if (!fks)
		return NULL;

	if (fks->len == 0)
		return fks;

	if (newlen == 0)
	{
		fks->len = fks->alloc = 0;
		free(fks->cstr);
		fks->cstr = NULL;
	}
	else if (newlen < fks->len)
	{
		fks->len = newlen;
		fks->cstr[newlen] = '\0';
		if ((fks->alloc > (newlen * _deflatefactor / 100)) && (fks->alloc > _minalloc))
		{
			char *newbuf;
			fks->alloc = newlen + 1;
			if (fks->alloc < _minalloc)
				fks->alloc = _minalloc;
			newbuf = realloc(fks->cstr, fks->alloc);
			if (!newbuf)
				fkpanic(FKSTRERR_MEMALLOC);
			fks->cstr = newbuf;
		}
	}
	return fks;
}

/*
 * Empty strings are left alone to keep the len == 0 invariant. Growth is to
 * exactly len + n + 1, not allocforlen(): the caller already said how much.
 */
fkstring *fkslack(fkstring *fks, size_t n)
{
	size_t	newalloc;
	char	*newbuf;

	if (!fks)
		return NULL;

	if (fks->len == 0)
		return fks;

	newalloc = fkaddlen(fkaddlen(fks->len, n), 1);
	if (fks->alloc < newalloc)
	{
		newbuf = realloc(fks->cstr, newalloc);
		if (!newbuf)
			fkpanic(FKSTRERR_MEMALLOC);
		fks->cstr = newbuf;
		fks->alloc = newalloc;
	}
	return fks;
}

/* Unlike fkstrtrunc(), ignores _deflatefactor and _minalloc. */
fkstring *fkfit(fkstring *fks)
{
	char	*newbuf;

	if (!fks)
		return NULL;

	if (fks->len != 0 && fks->alloc > fks->len + 1)
	{
		newbuf = realloc(fks->cstr, fks->len + 1);
		if (!newbuf)
			fkpanic(FKSTRERR_MEMALLOC);
		fks->cstr = newbuf;
		fks->alloc = fks->len + 1;
	}
	return fks;
}

fkstring *fkstrdup(const fkstring *fks)
{
	size_t		newlen, newalloc;
	fkstring	*newfkstr;

	if (!fks)
		return NULL;

	newlen = fks->len;
	if (newlen == 0)
		return fkstrnew(NULL);

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	newalloc = allocforlen(newlen);

	newfkstr->cstr = malloc(newalloc);
	if (!newfkstr->cstr)
		fkpanic(FKSTRERR_MEMALLOC);

	memcpy(newfkstr->cstr, fks->cstr, newlen + 1);
	newfkstr->len = newlen;
	newfkstr->alloc = newalloc;

	return newfkstr;
}

/* Shared by the fkstrcat*() and fkinsert*() families; appending is inserting
 * at pos == dst->len. Assumes pos <= dst->len; dst may have zero length.
 * src may point into dst's own buffer (e.g. fkstrcat(s, s)), so it's copied
 * aside first: growing dst can move the buffer, and shifting the tail can
 * overwrite src's bytes. */
static fkstring *fkinsert_internal(fkstring *dst, size_t pos, const char *src, size_t srclen)
{
	size_t newlen, newalloc;
	char *newbuf, *tmp = NULL;

	if (srclen == 0)
		return dst;

	newlen = fkaddlen(dst->len, srclen);

	if (dst->cstr && (uintptr_t)src >= (uintptr_t)dst->cstr &&
	    (uintptr_t)src < (uintptr_t)dst->cstr + dst->alloc)
	{
		tmp = malloc(srclen);
		if (!tmp)
			fkpanic(FKSTRERR_MEMALLOC);
		memcpy(tmp, src, srclen);
		src = tmp;
	}

	if (dst->alloc <= newlen)
	{
		newalloc = allocforlen(newlen);
		if (dst->len)
			newbuf = realloc(dst->cstr, newalloc);
		else
			newbuf = malloc(newalloc);

		if (!newbuf)
			fkpanic(FKSTRERR_MEMALLOC);

		dst->cstr = newbuf;
		dst->alloc = newalloc;
	}
	if (pos < dst->len)
		memmove(&dst->cstr[pos + srclen], &dst->cstr[pos], dst->len - pos);
	memcpy(&dst->cstr[pos], src, srclen);
	dst->len = newlen;
	dst->cstr[dst->len] = '\0';

	free(tmp);
	return dst;
}

fkstring *fkstrcat(fkstring *dst, const fkstring *src)
{
	return fkinsert_internal(dst, dst->len, src->cstr, src->len);
}

fkstring *fkstrcatc(fkstring *dst, const char *src)
{
	if (src && src[0])
		return fkinsert_internal(dst, dst->len, src, strlen(src));
	else
		return dst;
}

fkstring *fkstrcatone(fkstring *dst, char c)
{
	return fkinsert_internal(dst, dst->len, &c, 1);
}

/* pos == fks->len appends; pos > fks->len is an invalid argument. */
fkstring *fkinsert(fkstring *fks, size_t pos, const fkstring *src)
{
	if (!fks || !src || pos > fks->len)
		return NULL;

	return fkinsert_internal(fks, pos, src->cstr, src->len);
}

fkstring *fkinsertc(fkstring *fks, size_t pos, const char *cstr)
{
	if (!fks || !cstr || pos > fks->len)
		return NULL;

	return fkinsert_internal(fks, pos, cstr, strlen(cstr));
}

size_t fkremove(fkstring *fstr, size_t start, size_t len)
{
	if (!fstr)
		return 0;

	if ((start >= fstr->len) || (len == 0))
		return 0;

	if (len > (fstr->len - start))
		len = fstr->len - start;

	if (len < (fstr->len - start))
		memmove(&fstr->cstr[start], &fstr->cstr[start + len], fstr->len - start - len);

	if (!fkstrtrunc(fstr, fstr->len - len))
		return 0;

	return len;
}

/* Matches the \s character class from regular expressions, not the
 * locale-dependent isspace(). */
static int isfkspace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

size_t fkltrim(fkstring *fks)
{
	size_t count;

	if (!fks)
		return 0;

	count = 0;
	while (count < fks->len && isfkspace(fks->cstr[count]))
		count++;

	fkremove(fks, 0, count);
	return fks->len;
}

size_t fkrtrim(fkstring *fks)
{
	size_t newlen;

	if (!fks)
		return 0;

	newlen = fks->len;
	while (newlen > 0 && isfkspace(fks->cstr[newlen - 1]))
		newlen--;

	fkstrtrunc(fks, newlen);
	return fks->len;
}

size_t fktrim(fkstring *fks)
{
	if (!fks)
		return 0;

	fkrtrim(fks);
	fkltrim(fks);
	return fks->len;
}

fkstring *fksubstr(const fkstring *fstr, size_t start, size_t len)
{
	size_t		newalloc;
	fkstring	*newfkstr;

	if (!fstr)
		return NULL;

	if ((start >= fstr->len) || (len == 0))
		return fkstrnew(NULL);

	if (len > (fstr->len - start))
		len = fstr->len - start;

	newfkstr = malloc(sizeof(fkstring));
	if (!newfkstr)
		fkpanic(FKSTRERR_MEMALLOC);

	newalloc = allocforlen(len);

	newfkstr->cstr = malloc(newalloc);
	if (!newfkstr->cstr)
		fkpanic(FKSTRERR_MEMALLOC);

	memcpy(newfkstr->cstr, &fstr->cstr[start], len);
	newfkstr->cstr[len] = '\0';
	newfkstr->len = len;
	newfkstr->alloc = newalloc;

	return newfkstr;
}

fkstring **fksplit(const fkstring *src, char delim)
{
	size_t		nparts, i, start, partidx;
	fkstring	**result;

	if (!src)
		return NULL;

	nparts = 1;
	for (i = 0; i < src->len; i++)
		if (src->cstr[i] == delim)
			nparts++;

	result = malloc((nparts + 1) * sizeof(fkstring *));
	if (!result)
		fkpanic(FKSTRERR_MEMALLOC);

	start = 0;
	partidx = 0;
	for (i = 0; i < src->len; i++)
	{
		if (src->cstr[i] == delim)
		{
			result[partidx++] = fksubstr(src, start, i - start);
			start = i + 1;
		}
	}
	result[partidx++] = fksubstr(src, start, src->len - start);
	result[partidx] = NULL;

	return result;
}

void fkarraydestroy(fkstring **fka)
{
	size_t i;

	if (!fka)
		return;

	for (i = 0; fka[i]; i++)
		fkstrdestroy(fka[i]);

	free(fka);
}

/* Inverse of fksplit(): the total length is summed first (via fkaddlen())
 * so the result is allocated exactly once. A NULL sep joins with nothing. */
fkstring *fkjoin(fkstring **arr, const char *sep)
{
	size_t		seplen, total, i, pos;
	fkstring	*result;

	if (!arr)
		return NULL;

	seplen = sep ? strlen(sep) : 0;
	total = 0;
	for (i = 0; arr[i]; i++)
	{
		if (i > 0)
			total = fkaddlen(total, seplen);
		total = fkaddlen(total, arr[i]->len);
	}

	result = fkstrnewb(NULL, 0);
	if (total == 0)
		return result;

	result->alloc = allocforlen(total);
	result->cstr = malloc(result->alloc);
	if (!result->cstr)
		fkpanic(FKSTRERR_MEMALLOC);

	pos = 0;
	for (i = 0; arr[i]; i++)
	{
		if (i > 0 && seplen)
		{
			memcpy(result->cstr + pos, sep, seplen);
			pos += seplen;
		}
		if (arr[i]->len)
		{
			memcpy(result->cstr + pos, arr[i]->cstr, arr[i]->len);
			pos += arr[i]->len;
		}
	}
	result->cstr[pos] = '\0';
	result->len = pos;

	return result;
}

/* ASCII-only case folding, deliberately not the locale-dependent tolower()
 * (same rationale as isfkspace()). */
static unsigned char fkfoldcase(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

/* Shared by fkstrcmp()/fkstrcasecmp(): NULL sorts before any fkstring
 * (two NULLs are equal), then bytes are compared over the shorter length,
 * with ties broken by length. The cstr pointers are only touched when
 * minlen > 0, since an empty fkstring's cstr is NULL. */
static int fkstrcmp_internal(const fkstring *a, const fkstring *b, int fold)
{
	size_t	minlen, i;
	int	r;

	if (!a || !b)
		return (a ? 1 : 0) - (b ? 1 : 0);

	minlen = a->len < b->len ? a->len : b->len;
	if (minlen > 0)
	{
		if (fold)
		{
			for (i = 0; i < minlen; i++)
			{
				unsigned char ca = fkfoldcase(a->cstr[i]);
				unsigned char cb = fkfoldcase(b->cstr[i]);

				if (ca != cb)
					return ca < cb ? -1 : 1;
			}
		}
		else
		{
			r = memcmp(a->cstr, b->cstr, minlen);
			if (r)
				return r < 0 ? -1 : 1;
		}
	}

	if (a->len == b->len)
		return 0;
	return a->len < b->len ? -1 : 1;
}

int fkstrcmp(const fkstring *a, const fkstring *b)
{
	return fkstrcmp_internal(a, b, 0);
}

int fkstrcasecmp(const fkstring *a, const fkstring *b)
{
	return fkstrcmp_internal(a, b, 1);
}

int fkstreq(const fkstring *a, const fkstring *b)
{
	if (!a || !b)
		return a == b;

	if (a->len != b->len)
		return 0;

	return a->len == 0 || memcmp(a->cstr, b->cstr, a->len) == 0;
}

/* Assumes 0 < nlen <= haylen. */
static const char *fkmemmem(const char *hay, size_t haylen, const char *needle, size_t nlen)
{
#if FKSTR_HAVE_MEMMEM
	return memmem(hay, haylen, needle, nlen);
#else
	const char *p = hay;
	const char *last = hay + (haylen - nlen);	/* last position a match can start at */

	while (p <= last)
	{
		p = memchr(p, needle[0], last - p + 1);
		if (!p)
			return NULL;
		if (memcmp(p, needle, nlen) == 0)
			return p;
		p++;
	}
	return NULL;
#endif
}

/* Shared by fkstrfind()/fkstrfindc(). An empty needle matches at start, as
 * long as start <= hay->len. The cstr pointer is only touched when the
 * needle is non-empty and fits, since an empty fkstring's cstr is NULL. */
static size_t fkstrfind_internal(const fkstring *hay, const char *needle, size_t nlen, size_t start)
{
	const char *p;

	if (start > hay->len)
		return FKSTR_NPOS;
	if (nlen == 0)
		return start;
	if (nlen > hay->len - start)
		return FKSTR_NPOS;

	p = fkmemmem(&hay->cstr[start], hay->len - start, needle, nlen);
	return p ? (size_t)(p - hay->cstr) : FKSTR_NPOS;
}

size_t fkstrfind(const fkstring *hay, const fkstring *needle, size_t start)
{
	if (!hay || !needle)
		return FKSTR_NPOS;

	return fkstrfind_internal(hay, needle->cstr, needle->len, start);
}

size_t fkstrfindc(const fkstring *hay, const char *needle, size_t start)
{
	if (!hay || !needle)
		return FKSTR_NPOS;

	return fkstrfind_internal(hay, needle, strlen(needle), start);
}

size_t fkstrchr(const fkstring *fks, char c, size_t start)
{
	const char *p;

	if (!fks || start >= fks->len)
		return FKSTR_NPOS;

	p = memchr(&fks->cstr[start], c, fks->len - start);
	return p ? (size_t)(p - fks->cstr) : FKSTR_NPOS;
}

/* A plain loop rather than memrchr(), which is GNU-only. */
size_t fkstrrchr(const fkstring *fks, char c)
{
	size_t i;

	if (!fks)
		return FKSTR_NPOS;

	for (i = fks->len; i > 0; i--)
		if (fks->cstr[i - 1] == c)
			return i - 1;

	return FKSTR_NPOS;
}

/* Shared by fkstartswith()/fkstartswithc(). The length check comes first so
 * an empty prefix never touches fks->cstr, which is NULL for an empty string. */
static int fkstartswith_internal(const fkstring *fks, const char *prefix, size_t plen)
{
	if (plen > fks->len)
		return 0;

	return plen == 0 || memcmp(fks->cstr, prefix, plen) == 0;
}

int fkstartswith(const fkstring *fks, const fkstring *prefix)
{
	if (!fks || !prefix)
		return 0;

	return fkstartswith_internal(fks, prefix->cstr, prefix->len);
}

int fkstartswithc(const fkstring *fks, const char *prefix)
{
	if (!fks || !prefix)
		return 0;

	return fkstartswith_internal(fks, prefix, strlen(prefix));
}

/* Shared by fkendswith()/fkendswithc(). Checks slen <= fks->len before
 * computing fks->len - slen, to avoid unsigned wraparound. */
static int fkendswith_internal(const fkstring *fks, const char *suffix, size_t slen)
{
	if (slen > fks->len)
		return 0;

	return slen == 0 || memcmp(&fks->cstr[fks->len - slen], suffix, slen) == 0;
}

int fkendswith(const fkstring *fks, const fkstring *suffix)
{
	if (!fks || !suffix)
		return 0;

	return fkendswith_internal(fks, suffix->cstr, suffix->len);
}

int fkendswithc(const fkstring *fks, const char *suffix)
{
	if (!fks || !suffix)
		return 0;

	return fkendswith_internal(fks, suffix, strlen(suffix));
}

/* Shared by fkreplace()/fkreplacec(). Two passes: count the (non-overlapping,
 * left-to-right) matches, then build the result into one exactly-sized
 * buffer, so fks grows at most once. Building into a fresh buffer also makes
 * it safe for old/new to alias fks itself. max_count == 0 means no limit; an
 * empty old matches nothing. */
static fkstring *fkreplace_internal(fkstring *fks, const char *old, size_t oldlen,
				    const char *new, size_t newlen, size_t max_count)
{
	size_t	count, pos, match, reslen, resalloc, outpos, i;
	char	*buf;

	if (oldlen == 0)
		return fks;

	count = 0;
	pos = 0;
	while ((max_count == 0 || count < max_count) &&
	       (match = fkstrfind_internal(fks, old, oldlen, pos)) != FKSTR_NPOS)
	{
		count++;
		pos = match + oldlen;
	}
	if (count == 0)
		return fks;

	/* count * oldlen <= fks->len, so only the count * newlen side can overflow. */
	if (newlen && count > SIZE_MAX / newlen)
		fkpanic(FKSTRERR_OVERFLOW);
	reslen = fkaddlen(fks->len - count * oldlen, count * newlen);

	if (reslen == 0)
	{
		free(fks->cstr);
		fks->cstr = NULL;
		fks->len = fks->alloc = 0;
		return fks;
	}

	resalloc = allocforlen(reslen);
	buf = malloc(resalloc);
	if (!buf)
		fkpanic(FKSTRERR_MEMALLOC);

	pos = outpos = 0;
	for (i = 0; i < count; i++)
	{
		match = fkstrfind_internal(fks, old, oldlen, pos);
		memcpy(buf + outpos, fks->cstr + pos, match - pos);
		outpos += match - pos;
		if (newlen)
		{
			memcpy(buf + outpos, new, newlen);
			outpos += newlen;
		}
		pos = match + oldlen;
	}
	memcpy(buf + outpos, fks->cstr + pos, fks->len - pos);
	buf[reslen] = '\0';

	free(fks->cstr);
	fks->cstr = buf;
	fks->len = reslen;
	fks->alloc = resalloc;
	return fks;
}

fkstring *fkreplace(fkstring *fks, const fkstring *old, const fkstring *new, size_t max_count)
{
	if (!fks || !old || !new)
		return NULL;

	return fkreplace_internal(fks, old->cstr, old->len, new->cstr, new->len, max_count);
}

fkstring *fkreplacec(fkstring *fks, const char *old, const char *new, size_t max_count)
{
	if (!fks || !old || !new)
		return NULL;

	return fkreplace_internal(fks, old, strlen(old), new, strlen(new), max_count);
}
