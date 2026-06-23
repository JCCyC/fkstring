#ifndef __FKSTRING_INTERNAL_H__
#define __FKSTRING_INTERNAL_H__

extern int _bumpfactor;
extern int _deflatefactor;
extern int _minalloc;
extern int _sprintftry;
extern char *errmsgs[];

#define FKSTRERR_SUCCESS	0
#define FKSTRERR_MEMALLOC	1
#define FKSTRERR_VSNPRINTF	2

#define FKSTR_DEFAULT_BUMPFACTOR	143
#define FKSTR_DEFAULT_DEFLATEFACTOR	350
#define FKSTR_DEFAULT_MIN_ALLOC		16
#define FKSTR_DEFAULT_SPRINTF_TRY	(3*FKSTR_DEFAULT_MIN_ALLOC)

void fkpanic(int cause);
int allocforlen(int len);

#endif /* __FKSTRING_INTERNAL_H__ */
