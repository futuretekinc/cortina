#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "zlib.h"

static	unsigned char	inbuf[256*1024+1];
static	unsigned char	outbuf[256*1024];

int
main(int argc, char *argv[])
{
	z_stream zs;
	char *outfile;
	int c, fd, r, infd, outfd;

	infd = 0;
	outfd = 1;
	outfile = NULL;

	for(;;) {
		c = getopt(argc, argv, "i:o:h");
		if(c < 0)
			break;
		switch(c) {
		case '?':
		case 'h':
			fprintf(stderr, "usage :%s [-i infile] [-o outfile] [-h]\n", argv[0]);
			exit(1);
		case 'i':
			fd = open(optarg, O_RDONLY);
			if(fd < 0) {
				fprintf(stderr, "%s: open %s: %s\n", argv[0], optarg, strerror(errno));
				exit(1);
			}
			if(infd != 0)
				close(infd);
			infd = fd;
			break;
		case 'o':
			outfile = optarg;
			break;
		}
	}

	r = read(infd, inbuf, sizeof(inbuf));
	if(r < 0) {
		fprintf(stderr, "%s: read: %s\n", argv[0], strerror(errno));
		exit(1);
	}

	if(r > sizeof(inbuf)-1) {
		fprintf(stderr, "%s: read: input too big\n", argv[0]);
		exit(1);
	}

	memset(&zs, 0, sizeof(zs));
	zs.next_in = inbuf;
	zs.avail_in = r;
	zs.next_out = outbuf;
	zs.avail_out = sizeof(outbuf);

	r = inflateInit(&zs);
	if(r != Z_OK) {
		fprintf(stderr, "%s: inflateInit error %d\n", argv[0], r);
		exit(1);
	}

	r = inflate(&zs, Z_FINISH);
	switch(r) {
	default:
		fprintf(stderr, "%s: output error %d\n", argv[0], r);
		exit(1);
	case Z_OK:
		fprintf(stderr, "%s: output buffer too small\n", argv[0]);
		exit(1);
	case Z_STREAM_END:
		break;
	}

	if(outfile != NULL) {
		fd = open(outfile, O_WRONLY|O_CREAT|O_EXCL, 0644);
		if(fd < 0) {
			fprintf(stderr, "%s: open %s: %s\n", argv[0], outfile, strerror(errno));
			exit(1);
		}
		if(outfd != 1)
			close(outfd);
		outfd = fd;
	}

	r = write(outfd, outbuf, zs.total_out);
	if(r < 0) {
		fprintf(stderr, "%s: write: %s\n", argv[0], strerror(errno));
		exit(1);
	}

	if(r != zs.total_out) {
		fprintf(stderr, "%s: short write: %d expected %lu\n", argv[0], r, zs.total_out);
		exit(1);
	}

	fprintf(stderr, "input size : %lu bytes\n", zs.total_in);
	fprintf(stderr, "output size: %lu bytes\n", zs.total_out);

	close(infd);
	close(outfd);

	return 0;
}
