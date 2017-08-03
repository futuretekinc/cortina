/*
 * Modified from http://old.nabble.com/file/p26096804/ecdsa.c
 */
#include "secboot-ossl.h"
#include "sha.h"

/*
 * See Makefile for how this stuff is generated
 */
#include "plaintext.c"

int
main(void)
{
	unsigned char md[SHA256_DIGEST_LENGTH];

	SHA256(plaintext, sizeof(plaintext)-1, md);
	fatal(ERR_OCTVERIFY_SUCCESS);
}
