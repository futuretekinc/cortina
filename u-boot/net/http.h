//-----------------------------------------------------------------------------
// HTTP.H
//
//-----------------------------------------------------------------------------

#define NONE			0
#define GET_PAGE        	1
#define GET_JPEG             	2
#define POST_PAGE		3

#define	HTTP_START		0
#define	HTTP_GET		1
#define HTTP_POST		2
#define HTTP_HEAD_DATA		3
#define HTTP_UPLOAD		4
//#define HTTP_UPGRADE		5
#define HTTP_REBOOT		5

#define MAX_ETHER_PKT_LEN	(1536)

#define _HTTP_PROTOCOL 		"HTTP/1.1"
#define _HTTP_SERVER 		"Cortina Web v0.1"
#define _HTTP_TITLE 		"Simple HTTP Web Server"

#define HTTP_MAGIC 		0xa4411376
#define MAX_IMAGES		6

typedef struct 
{
	unsigned int 	upload_len;	/* upload file length with header */
	unsigned int 	rcv_len;	/* receive length */
	unsigned int 	data_len;	/* data length without header */
	unsigned char 	*buf;		/* upload file buffer */
} __attribute__ ((aligned(1), packed)) http_TCB;

typedef struct
{
	unsigned char	name[32];	/* image file name */
	unsigned int	major_version;	/* major version */
	unsigned int	minor_version;	/* minor version */
	unsigned int	start_address;	/* start offset address */
	unsigned int	size;		/* image size */
	unsigned int	part_address;	/* partition address in flash */
	unsigned int	part_size;	/* partition size in flash */
} header_info_t;

void init_http(void);
ushort http_server(uchar *inbuf, ushort header_len, ushort total_len,
		   uchar nr, uchar resend);

