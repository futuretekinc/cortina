/*
 * Modified from http://old.nabble.com/file/p26096804/ecdsa.c
 */
#include "secboot-ossl.h"
#include "sha.h"

static u8 secp112r1_pubkbuf[] =
{
	NID_secp112r1 >> 8,
	NID_secp112r1 & 0xFF,

#	include "secp112r1-ecdsa-pub-raw.c"
};

static u8 secp112r1_sigbuf[] =
{
#	include "secp112r1-signature.c"
};

static u8 secp128r1_pubkbuf[] =
{
	NID_secp128r1 >> 8,
	NID_secp128r1 & 0xFF,

#	include "secp128r1-ecdsa-pub-raw.c"
};

static u8 secp128r1_sigbuf[] =
{
#	include "secp128r1-signature.c"
};

static u8 secp256r1_pubkbuf[] =
{
	NID_X9_62_prime256v1 >> 8,			/* same as secp256r1 */
	NID_X9_62_prime256v1 & 0xFF,

#	include "secp256r1-ecdsa-pub-raw.c"
};

static u8 secp256r1_sigbuf[] =
{
#	include "secp256r1-signature.c"
};

static u8 secp384r1_pubkbuf[] =
{
	NID_secp384r1 >> 8,
	NID_secp384r1 & 0xFF,

#	include "secp384r1-ecdsa-pub-raw.c"
};

static u8 secp384r1_sigbuf[] =
{
#	include "secp384r1-signature.c"
};

static u8 secp521r1_pubkbuf[] =
{
	NID_secp521r1 >> 8,
	NID_secp521r1 & 0xFF,

#	include "secp521r1-ecdsa-pub-raw.c"
};

static u8 secp521r1_sigbuf[] =
{
#	include "secp521r1-signature.c"
};

#include "plaintext.c"

void
fatal(Sberr err)
{
#if defined(__arm__) || defined(__thumb__)
	__asm__ __volatile__ ("mov r0, %0" : : "r" (err) );
	__asm__ __volatile__ ("mov r7, #1");
	__asm__ __volatile__ ("svc #0x000000");
#else
	exit(err);		/* for profiling to work */
//	__asm__ __volatile__ ("mov   %0,%%ebx" : : "r" (err) );
//	__asm__ __volatile__ ("mov    $0x1,%eax");
//	__asm__ __volatile__ ("int    $0x80");
#endif

	for(;;);
}

int
main(void)
{
	int r;

	r = ecdsa_verify(	plaintext, sizeof(plaintext),
				secp112r1_sigbuf, sizeof(secp112r1_sigbuf),
				secp112r1_pubkbuf, sizeof(secp112r1_pubkbuf));
	if(r != ERR_VERIFY_SUCCESS)
		goto out;

	r = ecdsa_verify(	plaintext, sizeof(plaintext),
				secp128r1_sigbuf, sizeof(secp128r1_sigbuf),
				secp128r1_pubkbuf, sizeof(secp128r1_pubkbuf));
	if(r != ERR_VERIFY_SUCCESS)
		goto out;

	r = ecdsa_verify(	plaintext, sizeof(plaintext),
				secp256r1_sigbuf, sizeof(secp256r1_sigbuf),
				secp256r1_pubkbuf, sizeof(secp256r1_pubkbuf));
	if(r != ERR_VERIFY_SUCCESS)
		goto out;

	r = ecdsa_verify(	plaintext, sizeof(plaintext),
				secp384r1_sigbuf, sizeof(secp384r1_sigbuf),
				secp384r1_pubkbuf, sizeof(secp384r1_pubkbuf));
	if(r != ERR_VERIFY_SUCCESS)
		goto out;

	r = ecdsa_verify(	plaintext, sizeof(plaintext),
				secp521r1_sigbuf, sizeof(secp521r1_sigbuf),
				secp521r1_pubkbuf, sizeof(secp521r1_pubkbuf));
	if(r != ERR_VERIFY_SUCCESS)
		goto out;

out:
	fatal(r);
}
