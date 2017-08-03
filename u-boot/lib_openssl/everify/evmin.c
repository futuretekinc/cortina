/*
 * Modified from http://old.nabble.com/file/p26096804/ecdsa.c
 */
#include "secboot-ossl.h"
#include "sha.h"

typedef unsigned char u8;

u8 secp112r1_sigbuf[1];
u8 secp112r1_pubkbuf[1];
u8 plaintext[1];

int
octverify(const ECDSA_SIG *sig, const u8 *pubkbuf, int pubkbufsz, const void *data, int dsize)
{
	int curve;
	EC_KEY pub_key;
	EC_KEY *pubk, *pkey;
	u8 md[SHA256_DIGEST_LENGTH];
	const u8 *p, *ep;

	memset(&pub_key, 0, sizeof(pub_key));
	pub_key.version = 1;
	pub_key.conv_form = POINT_CONVERSION_UNCOMPRESSED;
	pub_key.references = 1;

	SHA256(data, dsize, md);

	p = pubkbuf;
	ep = pubkbuf + pubkbufsz;
	if(ep - p < 2)
		fatal(ERR_SHORT_PUBKEY);

	curve = (p[0] << 8) | p[1];
	p += 2;

	pub_key.group = EC_GROUP_new_by_curve_name(curve);
	if(pub_key.group == NULL)
		fatal(ERR_ECG_NEW);

	pubk = &pub_key;
	pkey = o2i_ECPublicKey(&pubk, &p, ep - p);
	if(pkey != pubk)
		fatal(ERR_O2I_ECPUBKEY);

	return ecdsa_do_verify(md, sizeof(md), sig, &pub_key);
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

void
verify(const u8 *sigbuf, int sigbufsz, const u8 *pubkbuf, int pubkbufsz)
{
	static int iteration = 100;

	int r;

	ECDSA_SIG sig;
	const u8 *p, *ep;

	memset(&sig, 0, sizeof(sig));

	p = sigbuf;
	ep = sigbuf + sigbufsz;
	r = bnload(p, ep, &sig.r);
	if(r < 0)
		fatal(iteration+ERR_BNLOAD_R);
	p += r;

	r = bnload(p, ep, &sig.s);
	if(r < 0)
		fatal(iteration+ERR_BNLOAD_S);

	r = octverify(&sig, pubkbuf, pubkbufsz, plaintext, sizeof(plaintext));
	switch(r) {
	default:
	case -1:
		fatal(iteration+ERR_OCTVERIFY_ERROR);
	case 0:
		fatal(iteration+ERR_OCTVERIFY_FAIL);
	case 1:
		break;
	}

	iteration++;
}

int
main(void)
{
	verify(secp112r1_sigbuf, sizeof(secp112r1_sigbuf), secp112r1_pubkbuf, sizeof(secp112r1_pubkbuf));

	fatal(ERR_OCTVERIFY_SUCCESS);
}
