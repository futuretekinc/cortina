#include "secboot-ossl.h"

u8	pktbuf[192*1024];
u8	outbuf[64*1024];

static void*
pktzalloc(void *opaque, unsigned int nitems, unsigned int size)
{
	static unsigned long nalloc;
	static u8 *pktpos = pktbuf;

	unsigned int n;

	n = nitems*size;
	if(nalloc+n > 192*1024)
		return NULL;

	opaque = pktpos;
	pktpos += n;
	nalloc += n;

	return opaque;
}

static void
pktzfree(void *opaque, void *buf, unsigned int size)
{
	/* NOOP, pktram is reused later, so no need to "free" */
	fatal(50);
}

#if 0
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

void __attribute__ ((__noreturn__))
fatal(Sberr err) 
{
#ifdef __arm__
	__asm__ __volatile__ ("mov r0, %0" : : "r" (err) );
	__asm__ __volatile__ ("mov r7, #1");
	__asm__ __volatile__ ("svc #0x000000");
#else
	__asm__ __volatile__ ("mov   %0,%%ebx" : : "r" (err) );
	__asm__ __volatile__ ("mov    $0x1,%eax");
	__asm__ __volatile__ ("int    $0x80");
#endif
	for(;;);
}

void
raise(int n)
{
	fatal(51);
}

void*
malloc(int n)
{
	fatal(100);
}

void*
free(void *arg)
{
	fatal(101);
}

void*
calloc(int size, int nmemb)
{
	fatal(102);
}

#include "../sys/zlib.c"
#define NOSKIP
#include "/space/design/sw/platforms/g2/openwrt-10.03/build_dir/target-arm-openwrt-linux-uclibcgnueabi/secure-boot/openssl/deflated_ecdsa.c"

static void
check(z_stream *zst)
{
	int n, i;
	unsigned char *sp, *p, *ep;
	unsigned long whole, part, start;

	sp = zst->next_in;
	p = sp;
	ep = p + zst->avail_in;

	start = adler32(0L, Z_NULL, 0);
	whole = start;
	while(p < ep) {
		n = 256;
		if(n > ep - p)
			n = ep - p;
		part = adler32(start, p, n);
		whole = adler32(whole, p, n);
		printf("part sum %08x addr %p off %d\n", part, p, p - sp);
		p += n;
	}
	printf("whole %08x\n", whole);

	for(i = 8192; i < 8192+1024; i++) {
		if((i % 32) == 0)
			printf("\n");
		printf("%02x ", sp[i]);
	}
	printf("\n");
}

int
main(void)
{
	int r;
	static z_stream zst;

	zst.zalloc = pktzalloc;
	zst.zfree = pktzfree;
	zst.opaque = Z_NULL;
	zst.next_in = deflated_ecdsa;
	zst.avail_in = deflated_ecdsa_size;
	zst.next_out = outbuf;
	zst.avail_out = sizeof(outbuf);

	check(&zst);

	r = inflateInit(&zst);
	if(r != Z_OK)
		fatal(r);

	r = inflate(&zst, Z_FINISH);
	if(r != Z_STREAM_END)
		fatal(r);
	fatal(99);
}
