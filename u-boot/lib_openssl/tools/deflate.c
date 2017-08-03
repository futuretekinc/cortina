#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <zlib.h>

static	unsigned char	inbuf[256*1024+1];
static	unsigned char	outbuf[256*1024];

int
main(int argc, char *argv[])
{
	FILE *of;
	z_stream zs;
	int c, fd, lvl, r, n;
	int infd, outfd, complvl;
	unsigned char *op, *oep;
	char *p, *outfile, *cname;

	infd = 0;
	outfd = 1;
	cname = NULL;
	outfile = NULL;
	complvl = Z_DEFAULT_COMPRESSION;

	for(;;) {
		c = getopt(argc, argv, "c:i:o:l:h");
		if(c < 0)
			break;
		switch(c) {
		case '?':
		case 'h':
			fprintf(stderr, "usage :%s [-c] [-i infile] [-o outfile] [-l 0..9] [-h]\n", argv[0]);
			exit(1);
		case 'c':
			if(optarg[0] == '-') {
				fprintf(stderr, "%s: option requires an argument -- c\n", argv[0]);
				exit(1);
			}
			cname = optarg;
			break;
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
		case 'l':
			lvl = strtol(optarg, &p, 0);
			if(p == optarg || *p != '\0') {
				fprintf(stderr, "%s: bad level\n", argv[0]);
				exit(1);
			}
			if(lvl < 0 || lvl > 9) {
				fprintf(stderr, "%s: level out of range (0..9)\n", argv[0]);
				exit(1);
			}
			complvl = lvl;
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

	r = deflateInit(&zs, complvl);
	if(r != Z_OK) {
		fprintf(stderr, "%s: deflateInit error %d (%s)\n", argv[0], r, zError(r));
		exit(1);
	}

	r = deflate(&zs, Z_FINISH);
	switch(r) {
	default:
		fprintf(stderr, "%s: output error %d (%s)\n", argv[0], r, zError(r));
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

	if(cname != NULL) {
		of = fdopen(outfd, "w");
		if(of == NULL) {
			fprintf(stderr, "%s: fdopen: %s\n", argv[0], strerror(errno));
			exit(1);
		}

		fprintf(of, "const unsigned char %s[] = \n", cname);
		fprintf(of, "{\n\t");
		
		op = outbuf;
		oep = outbuf + zs.total_out;
		n = 0;
		for(; op < oep; op++) {
			fprintf(of, "0x%02x,", *op);
			n++;
			if(n < 16)
				continue;
			n = 0;
			fprintf(of, "\n\t");
		}
		fprintf(of, "\n};\n");
		fprintf(of, "const unsigned long	%s_size = sizeof(%s);\n", cname, cname);
		fflush(of);
	}
	else {
		r = write(outfd, outbuf, zs.total_out);
		if(r < 0) {
			fprintf(stderr, "%s: write: %s\n", argv[0], strerror(errno));
			exit(1);
		}

		if(r != zs.total_out) {
			fprintf(stderr, "%s: short write: %d expected %lu\n", argv[0], r, zs.total_out);
			exit(1);
		}
	}

	fprintf(stderr, "input size : %lu bytes\n", zs.total_in);
	fprintf(stderr, "output size: %lu bytes\n", zs.total_out);

	close(infd);
	close(outfd);

	return 0;
}
