#include "./include/secboot-ossl.h"
#include "./include/sha.h"
#include "ossl-api.h"

static unsigned short curvemap[] =
{
	[CURVE_secp112r1] NID_secp112r1,
	[CURVE_secp128r1] NID_secp128r1,
	[CURVE_secp256r1] NID_X9_62_prime256v1,	// OpenSSL name for secp256r1
	[CURVE_secp384r1] NID_secp384r1,
	[CURVE_secp521r1] NID_secp521r1,
};

/*
 * Note the malloc implementation is exremely bare-bones, 
 * and is carefully tuned to only work with ecdsa_verify().
 * Specifically:  There is NO free(), instead ecdsa_verify()
 * resets the heap to the default state just prior to returning,
 * in effect, this free's all memory allocated with previous
 * malloc() calls.
 */
static	unsigned char	mallocbuf[22*1024];
static	unsigned char*	mpos = mallocbuf;
static	unsigned char*	mtop = mallocbuf + sizeof(mallocbuf);

static	int	bnload(const u8 *p, const u8 *ep, BIGNUM **ret);
static	int	octverify(	const ECDSA_SIG *sig, unsigned short curve,
				const u8 *pubkbuf, int pubkbufsz,
				const void *data, int dsize);

void*
CRYPTO_malloc(int num, const char *file, int line)
{
	int sz;
	void *p;

	sz = (num + 3) & ~3;

	if(mtop - mpos < sz)
		return NULL;

	p = mpos;
	mpos += sz;
	
	memset(p, 0, sz);

	return p;
}

void
CRYPTO_free(void *buf)
{
}

int
ecdsa_verify(	const void* plain, int plainsz,
		const u8 *sigbuf, int sigbufsz,
		const u8 *pubkbuf, int pubkbufsz)
{
	int r;
	ECDSA_SIG sig;
	const u8 *p, *ep;
	unsigned short curve;

	memset(&sig, 0, sizeof(sig));

	p = sigbuf;
	ep = sigbuf + sigbufsz;
	r = bnload(p, ep, &sig.r);
	if(r < 0)
		return ERR_BNLOAD_R;
	p += r;

	r = bnload(p, ep, &sig.s);
	if(r < 0)
		return ERR_BNLOAD_S;

	if(pubkbufsz < 1)
		return ERR_SHORT_PUBKEY;
	curve = *pubkbuf++;
	pubkbufsz--;
	if(curve >= nelem(curvemap) || curvemap[curve] == 0)
		return ERR_BAD_CURVEID;
	curve = curvemap[curve];

	r = octverify(&sig, curve, pubkbuf, pubkbufsz, plain, plainsz);

	mpos = mallocbuf;	// reset heap to empty state (see note above)
	
	return r;
}
#if 0
int
raise(int sig)
{
	return 0;
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
#endif
time_t
time(time_t *t)
{
	return 0;
}

int
CRYPTO_add_lock(int *pointer, int amount, int type, const char *file, int line)
{
	return 0;
}

void
CRYPTO_lock(int mode, int type,const char *file, int line)
{
}

int
BIO_snprintf(char *buf, size_t n, const char *format, ...)
{
	return 0;
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
		return;

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
//	fatal(ERR_RAND_ADD);
}

int
RAND_bytes(unsigned char *buf, int num)
{
	return 0;
//	fatal(ERR_RAND_BYTES);
}


void
RAND_pseudo_bytes(unsigned char *buf, int num)
{
//	fatal(ERR_RAND_PSEUDO_BYTES);
}

void
OPENSSL_cleanse(void *ptr, size_t len)
{
	memset(ptr, 0, len);
}

static	int
octverify(	const ECDSA_SIG *sig, unsigned short curve,
		const u8 *pubkbuf, int pubkbufsz,
		const void *data, int dsize)
{
	int r;
	EC_KEY pub_key;
	const u8 *p, *ep;
	EC_KEY *pubk, *pkey;
	u8 md[SHA256_DIGEST_LENGTH];

	memset(&pub_key, 0, sizeof(pub_key));
	pub_key.version = 1;
	pub_key.conv_form = POINT_CONVERSION_UNCOMPRESSED;
	pub_key.references = 1;

	SHA256(data, dsize, md);

	p = pubkbuf;
	ep = pubkbuf + pubkbufsz;

	pub_key.group = EC_GROUP_new_by_curve_name(curve);
	if(pub_key.group == NULL)
		return ERR_ECG_NEW;

	pubk = &pub_key;
	pkey = o2i_ECPublicKey(&pubk, &p, ep - p);
	if(pkey != pubk)
		return ERR_O2I_ECPUBKEY;

	r = ecdsa_do_verify(md, sizeof(md), sig, &pub_key);
	switch(r) {
	default:
	case -1:
		return ERR_VERIFY_ERROR;
	case 0:
		return ERR_VERIFY_FAIL;
	case 1:
		break;
	}

	return ERR_VERIFY_SUCCESS;
}

static int
bnload(const u8 *p, const u8 *ep, BIGNUM **ret)
{
	int len;
	BIGNUM *bn;

	len = *p++;
	if(ep - p < len)
		return -1;

	bn = BN_bin2bn(p, len, *ret);					/* OK, not too bad... can probably be simplified though... */
	if(bn == NULL)
		return -1;

	*ret = bn;
	return len+1;
}

