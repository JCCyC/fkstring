#ifndef __FKSTRING_INTERNAL_H__
#define __FKSTRING_INTERNAL_H__

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int _bumpfactor;
extern int _deflatefactor;
extern size_t _minalloc;
extern size_t _sprintftry;
extern size_t _slurptry;
extern const char *errmsgs[];

#define FKSTRERR_SUCCESS	0
#define FKSTRERR_MEMALLOC	1
#define FKSTRERR_VSNPRINTF	2
#define FKSTRERR_OVERFLOW	3

#define FKSTR_DEFAULT_BUMPFACTOR	143
#define FKSTR_DEFAULT_DEFLATEFACTOR	350
#define FKSTR_DEFAULT_MIN_ALLOC		16
#define FKSTR_DEFAULT_SPRINTF_TRY	(3*FKSTR_DEFAULT_MIN_ALLOC)
#define FKSTR_DEFAULT_SLURP_TRY		4096

static inline void fkpanic(int cause)
{
	if (cause)
		if (write(2, errmsgs[cause], strlen(errmsgs[cause]))) {}	/* nothing to do on failure; if() silences -Wunused-result */
	exit(253);
}

/* Checked a + b for length arithmetic: overflow is as unrecoverable as OOM. */
static inline size_t fkaddlen(size_t a, size_t b)
{
	if (b > SIZE_MAX - a)
		fkpanic(FKSTRERR_OVERFLOW);
	return a + b;
}

/* Checked a * b, same policy as fkaddlen(). */
static inline size_t fkmullen(size_t a, size_t b)
{
	if (a && b > SIZE_MAX / a)
		fkpanic(FKSTRERR_OVERFLOW);
	return a * b;
}

static inline size_t allocforlen(size_t len)
{
	size_t proposed = (len * _bumpfactor / 100) + 1;

	if (proposed < _minalloc)
		proposed = _minalloc;
	return proposed;
}

#endif /* __FKSTRING_INTERNAL_H__ */
