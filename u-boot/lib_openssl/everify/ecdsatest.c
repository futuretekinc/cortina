#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "secboot-ossl.h"
#include "include/sha.h"

#include <stdarg.h>

static char pubkey[] =
	"-----BEGIN PUBLIC KEY-----\n"
	"MD4wEAYHKoZIzj0CAQYFK4EEAAkDKgAEIlzYflxD0396M0i6dGfSY3khTU7kiNyE\n"
	"v/B1EoyGmqvH7tjhSmpP1A==\n"
	"-----END PUBLIC KEY-----\n";

static const char sigr[] = "DAC934B7D547B6B47C6AB2AFA4A8AA90DA95ED9E";
static const char sigs[] = "F0B88ECB9D0389052850B5CD9CA9E1DE7154CDB8";

static unsigned char defplaintext[] = "this is the plaintext";

void
flushit(void)
{
//	fflush(stdout);
}

unsigned long nmalloc;
void
CRYPTO_free(void *buf)
{
	free(buf);
}

void*
CRYPTO_malloc(int num, const char *file, int line)
{
	void *p;
	nmalloc += num;

//printf("malloc %d from %s:%d\n", num, file, line);

	p = malloc(num);
	if(p == NULL) {
		printf("malloc failed for %d bytes from %s:%d\n", num, file, line);
		exit(1);
	}
	if(p != NULL)
		memset(p, 0, num);

	return p;
}

int
CRYPTO_add_lock(int *pointer, int amount, int type, const char *file, int line)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

void
CRYPTO_lock(int mode, int type,const char *file, int line)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

int
BIO_snprintf(char *buf, size_t n, const char *format, ...)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

void
ERR_clear_error(void)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

unsigned long
ERR_peek_last_error(void)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

void
RAND_add(const void *buf, int num, double entropy)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

int
RAND_bytes(unsigned char *buf, int num)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}


void
RAND_pseudo_bytes(unsigned char *buf, int num)
{
	fprintf(stderr, "unimplemented: %s:%d\n", __FILE__, __LINE__);
	exit(1);
}

void
OPENSSL_cleanse(void *ptr, size_t len)
{
	memset(ptr, 0, len);
}

static void
getsig(ECDSA_SIG *sig)
{
	int r;

	r = BN_hex2bn(&sig->r, sigr);
	if(r <= 0) {
		printf("convert sig.r failed\n");
		exit(1);
	}

	r = BN_hex2bn(&sig->s, sigs);
	if(r <= 0) {
		printf("convert sig.s failed\n");
		exit(1);
	}
}

EC_KEY*
getpubkey(void)
{
	BIO *in;
	EC_KEY *key;

	in = BIO_new_mem_buf(pubkey, sizeof(pubkey));
	if(in == NULL) {
		printf("bio new failed\n");
		exit(1);
	}

	key = PEM_read_bio_EC_PUBKEY(in, NULL, NULL, NULL);
	if(key == NULL) {
		printf("read pubkey failed\n");
		exit(1);
	}

	return key;
}

int
getplaintext(int argc, char **argv, char **ptxt)
{
	int len, n, r;
	char *plaintext, *p;

	if(argc <= 1) {
		*ptxt = defplaintext;
		return strlen(defplaintext);
	}

	len = atoi(argv[1]);
	plaintext = malloc(len);
	if(plaintext == NULL) {
		printf("malloc fail\n");
		exit(1);
	}
	n = len;
	p = plaintext;
	while(n > 0) {
		r = read(0, p, n);
		if(r < 0) {
			printf("read failed %s\n", strerror(errno));
			return 1;
		}
		if(r == 0) {
			printf("short read need %d more\n", n);
			return 1;
		}
		p += r;
		n -= r;
	}

	*ptxt = plaintext;
}

int
main(int argc, char *argv[])
{
	EC_KEY *pub;
	ECDSA_SIG sig;
	char *plaintext;
	int r, i, len, n;
	unsigned char md[SHA256_DIGEST_LENGTH], *p;

	len = getplaintext(argc, argv, &plaintext);

	printf("plaintext len %d\n", len);

	SHA256(plaintext, len, md);
	printf("SHA256 digest ");
	for(i = 0; i < sizeof(md); i++)
		printf("%02x", md[i]);
	printf("\n");

	memset(&sig, 0, sizeof(sig));
	getsig(&sig);

	printf("load pub key\n");
	pub = getpubkey();

	printf("num malloc    %lu\n", nmalloc);

	printf("ecdsa verify  ");
//	r = ecdsa_do_verify(md, sizeof(md), const ECDSA_SIG *sig, EC_KEY *eckey);
	r = ecdsa_do_verify(md, sizeof(md), &sig, pub);
	if(r < 0) {
		printf("failed\n");
		return 1;
	}
	printf("success\n");

	return 0;
}
