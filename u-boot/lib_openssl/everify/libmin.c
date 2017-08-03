#include "secboot-ossl.h"

static	unsigned char	mallocbuf[10*1024];
static	unsigned char*	mpos = mallocbuf;
static	unsigned char*	mtop = mallocbuf + sizeof(mallocbuf);

int
raise(int sig)
{
	for(;;);
}
	
void*
memset(void *s, int c, size_t n)
{
	unsigned char *p, *e;

	p = s;
	e = p + n;
	while(p < e)
		*p++ = c;

	return s;
}

void*
memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *p, *e;
	const unsigned char *s;

	s = src;
	p = dest;
	e = p + n;

	while(p < e)
		*p++ = *s++;

	return dest;
}

time_t
time(time_t *t)
{
	fatal(ERR_TIME);
}

void
fatal(Sberr err)
{
	for(;;);
}

void
CRYPTO_free(void *buf)
{
//	free(buf);
}

void*
CRYPTO_malloc(int num, const char *file, int line)
{
	int sz;
	void *p;

	sz = (num + 3) & ~3;

	if(mtop - mpos < sz)
		fatal(ERR_MALLOC);

	p = mpos;
	mpos += sz;
	
	memset(p, 0, sz);

	return p;
}

int
CRYPTO_add_lock(int *pointer, int amount, int type, const char *file, int line)
{
	fatal(ERR_CRYPTO_ADD_LOCK);
}

void
CRYPTO_lock(int mode, int type,const char *file, int line)
{
	fatal(ERR_CRYPTO_LOCK);
}

int
BIO_snprintf(char *buf, size_t n, const char *format, ...)
{
	fatal(ERR_BIO_SNPRINTF);
}

struct Errstk
{
	unsigned long	error;
	const char*	file;
	int		line;
};

static	struct Errstk	errstk[10];
static	int		errlvl;

void
ERR_put_error(int lib, int func, int reason, const char *file, int line)
{
	struct Errstk *es;

	if(errlvl >= nelem(errstk))
		fatal(ERR_ERR_PUT_ERROR);

	es = &errstk[errlvl];
	es->error = ERR_PACK(lib, func, reason);
	es->file = file;
	es->line = line;
	errlvl++;
}

void
ERR_clear_error(void)
{
	errlvl = 0;
}

unsigned long
ERR_peek_last_error(void)
{
	if(errlvl <= 0)
		return 0;

	return errstk[errlvl-1].error;

}

void
RAND_add(const void *buf, int num, double entropy)
{
	fatal(ERR_RAND_ADD);
}

int
RAND_bytes(unsigned char *buf, int num)
{
	fatal(ERR_RAND_BYTES);
}


void
RAND_pseudo_bytes(unsigned char *buf, int num)
{
	fatal(ERR_RAND_PSEUDO_BYTES);
}

void
OPENSSL_cleanse(void *ptr, size_t len)
{
	memset(ptr, 0, len);
}

