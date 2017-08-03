#ifndef _OSSL_API_H_
#define _OSSL_API_H_
/*
 * Interface to the code that is inflated from ROM and
 * copied into the PKTRAM
 *
 *
 * The entire OSSL library is compiled in THUMB2.  GCC does the
 * appropriate internetworking on ARMv7 architectures to
 * automatically jump between ARM and THUMB mode as long as the
 * symbol contains a 1 in the low bit.
 *
 * See Table 5, Note "[a]" in the ARM instruction reference for details:
 *
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489b/CIHGJHED.html
 *
 *	[a] For word loads, Rt can be the PC. A load to the PC causes
 *	    a branch to the address loaded. In ARMv4, bits[1:0] of the
 *	    address loaded must be 0b00. In ARMv5T and above, bits[1:0]
 *	    must not be 0b10, and if bit[0] is 1, execution continues
 *	    in Thumb state, otherwise execution continues in ARM state.
 */

/*
 * Supported curves.  See the "booting g2" spec for details
 */
typedef enum
{
	CURVE_secp112r1	= 1,
	CURVE_secp128r1,
	CURVE_secp256r1,
	CURVE_secp384r1,
	CURVE_secp521r1,
} Curve;
	

typedef struct OSSLAPI OSSLAPI;
struct OSSLAPI
{
	/*
	 * IMPORTANT NOTE:
	 *	as currently written that this structure must
	 *	only contain function pointers.  See ossl_init()
	 *	for details, but the summary is that the structure
	 *	is converted to an array-of-u32 to bitwise-or in the
	 *	"thumb mode" indicator bit to simplify life for the API
	 *	user (see THUMB2 notes above)
	 */
	int		(*ecdsa_verify)(const void *plain, int plainsz,
					const unsigned char *sig, int sigsz,
					const unsigned char *pubk, int pubksz) __attribute__ ((__long_call__));
	unsigned char*	(*SHA256)(const unsigned char *d,
				 size_t n,
				 unsigned char *md) __attribute__ ((__long_call__));
};


/*
 * Helper routines to minimize casting exposure to API users
 */
#define THUMBIFY(addr)		((void*)((unsigned long)(addr) | 1))
#define UN_THUMBIFY(addr)	((void*)((unsigned long)(addr) & ~1))


/*
 * The caller should define the "ossl_init" symbol value as follows:
 *
 *	.text
 *	.global	ossl_init
 *	.type	ossl_init %function
 *	.equ	ossl_init, 0xF6A30000+1	// +1 to switch to thumb mode
 *
 * And the C prototype below guarantees the arm compiler will do the
 * call correctly via a in instruction similar to this:
 *
 *	ldr	pc, [r12]
 *
 */

extern	OSSLAPI*	ossl_init(void) __attribute__ ((__long_call__));

#endif /* _OSSL_API_H_ */
