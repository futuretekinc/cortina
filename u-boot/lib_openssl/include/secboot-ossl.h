#ifndef _SECBOOT_OSSL_H_
#define _SECBOOT_OSSL_H_

#define	nelem(x)	(sizeof(x)/sizeof((x)[0]))

typedef unsigned char u8;

typedef enum Sberr Sberr;
enum Sberr
{
	ERR_VERIFY_SUCCESS = 0,
	ERR_MALLOC,
	ERR_CRYPTO_ADD_LOCK,
	ERR_CRYPTO_LOCK,
	ERR_BIO_SNPRINTF,
	ERR_ERR_PUT_ERROR,
	ERR_RAND_ADD,
	ERR_RAND_BYTES,
	ERR_RAND_PSEUDO_BYTES,
	ERR_SHORT_PUBKEY,
	ERR_ECG_NEW,
	ERR_O2I_ECPUBKEY,
	ERR_BNLOAD_R,
	ERR_BNLOAD_S,
	ERR_TIME,
	ERR_AEABI_UIDIV,
	ERR_AEABI_ULDIVMOD,
	ERR_EABI_IDIV,
	ERR_AEABI_UIDIVMOD,
	ERR_RAISE,
	ERR_VERIFY_ERROR,
	ERR_VERIFY_FAIL,
	ERR_EC_WNAF_MUL,
	ERR_CURVE_TWO_FIELD,
	ERR_BN_MOD_CONSTTIME,
	ERR_GFP_SET_SIMPLE,
	ERR_GFP_POINT2OCT,
	ERR_GFP_SIMPLE_CMP,
	ERR_GFP_CHECK_DISC,
	ERR_GFP_GET_JPRO,
	ERR_GFP_GRP_GET_CURVE,
	ERR_GFP_MAKE_AFFINE,
	ERR_GFP_MONT_GRPCPY,
	ERR_BAD_CURVEID,
} ;

void	fatal(Sberr err) __attribute__ ((__noreturn__));

#if defined(__x86_64__)
# define SIXTY_FOUR_BIT
#else
# define THIRTY_TWO_BIT
#endif

#define	DATA_ORDER_IS_BIG_ENDIAN

#undef	BN_LLONG
#define OPENSSL_SMALL_FOOTPRINT
#define OPENSSL_NO_ENGINE
#define OPENSSL_NO_FP_API
#define OPENSSL_NO_BIO
#define OPENSSL_NO_LHASH
#define OPENSSL_NO_ASM
#define OPENSSL_NO_INLINE_ASM
#define OPENSSL_NO_STDIO
#define OPENSSL_NO_ERR

#ifndef _STDIO_H
#define NULL	((void*)0)
#endif

#ifndef _ASSERT_H
#define assert(x)
#endif

#define CRYPTO_EX_INDEX_ECDSA		12

typedef unsigned int size_t;

#ifndef _TIME_H
typedef unsigned int time_t;
#endif

extern	void*	memset(void *s, int c, size_t n);
extern	void*	memcpy(void *dest, const void *src, size_t n);
extern	void*	memchr(const void *s, int c, size_t n);

#define	INT_MAX	2147483647

#include "ossl_typ.h"
#include "../bn/bn.h"
#include "../ec/ec.h"
#include "../ecdsa/ecdsa.h"
#include "../asn1/asn1.h"
#include "obj_mac.h"
#include "../ec/ec_lcl.h"
#include "stack.h"
#include "safestack.h"
#include "crypto.h"
#include "../err/err.h"

extern	int	ecdsa_do_verify(	const unsigned char *dgst, int dgst_len,
					const ECDSA_SIG *sig, EC_KEY *eckey);

extern	int	ecdsa_verify(	const void* plain, int plainsz,
				const u8 *sigbuf, int sigbufsz,
				const u8 *pubkbuf, int pubkbufsz);

#endif /* _SECBOOT_OSSL_H_ */
