#include "./include/secboot-ossl.h"
#include "./include/sha.h"
#include "ossl-api.h"

/*
 * Zero bss and return pointer to the exported APIs
 */
OSSLAPI* __attribute__ ((__long_call__))
ossl_init(void)
{
	//extern u8 _edata[], _end[];

	static OSSLAPI api =
	{
		.ecdsa_verify	= ecdsa_verify,
		.SHA256		= SHA256,
	};

	int i;

	//memset(_edata, 0, _end - _edata);

	return &api;
}
