/*
 * Net HTTP.C
 *
 * This module is the Web Server
 * The HTTP protocol specification is at http://www.w3.org/Protocols/ 
 */
 
#include <common.h>
#include <net.h>
#include <malloc.h>
#include "tcp.h"
#include "http.h"
#include "webpage.h"

#define	ETHER_IP_HLEN	(ETHER_HDR_SIZE + IP_HDR_SIZE_NO_UDP)

const char 	c_len[] 	= "Content-Length: ";
const char 	c_headerend[] 	= "\r\n\r\n";
const char 	c_boundary[] 	= "boundary=";
const char 	c_fname[] 	= "filename=";

static uint	http_state = HTTP_START;
static char	boundary_code[64];

static char	http_buf[PKTSIZE_ALIGN];
static char	*upgrade_buffer = (char *)0x5000000;

http_TCB	post_info;

/* These structures keep track of connection information */
extern TCP_CONNECTION conxn[];
 
extern IPaddr_t	NetOurIP;	/* Our IP addr (0 = unknown) */
static char text[20];

#define	DEBUG	0x99
static uchar  http_debug;

static int http_parse_post(uchar *tcp_data, uint *content_length);
static int http_parse_mime(uchar *tcp_data, ushort tcp_data_len, uchar nr);
void http_send(uchar *outbuf, uint len, uchar nr);
extern int upgrade_flash(void *buffer, ulong addr, int sz, uint psz);
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);


int http_check_image(unsigned char *buf, unsigned int	size)
{
	unsigned int	magic;

	magic = *(int *)buf;
	if (magic != HTTP_MAGIC) {
		printf("Invalid image file !!!(%x)\n",magic);
		return (1);
	}
	return (0);	
}

int do_upgrade_image(unsigned char *buf, unsigned int	size)
{
	header_info_t	*header;
	unsigned char	*ptr;
	unsigned int	magic;
	unsigned int	parts;
	unsigned int	i;
	int		ret;
	
	ptr = buf;
	magic = *(int *)ptr;
	if (magic != HTTP_MAGIC) {
		printf("Invalid image file !!!(%x)\n",magic);
		return (1);
	} 
	ptr = ptr + sizeof(int);
	parts = *(int *)ptr;
	ptr = ptr + sizeof(int);
	
	for (i=0; i<parts; i++) {
		header = (header_info_t *)ptr;
		printf("name = %s\n",header->name);
		printf("major_version = %d\n",header->major_version);
		printf("minor_version = %d\n",header->minor_version);
		printf("start_address = 0x%x\n",header->start_address);
		printf("size = 0x%x\n",header->size);
		printf("part_address = 0x%x\n",header->part_address);
		printf("part_size = 0x%x\n\n\n",header->part_size);
		
		ret = upgrade_flash(&buf[header->start_address], 
			header->part_address, header->size, header->part_size);
		printf("Upgrade %s ......done !!!\n",header->name);
		
		ptr = ptr + sizeof(header_info_t);
	}
	return (0);
}

char *memstr(const char *s1, const char *s2, int len)
{
	int l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *) s1;
	/* l1 = strlen(s1); */
	l1 = len;
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1,s2,l2))
			return (char *) s1;
		s1++;
	}
	return NULL;
}
 
#if 0
/*------------------------------------------------------------------------
 * This function converts an integer to an ASCII string.  It is a 
 * normally provided as a standard library function but the Keil
 * libraries do not include it.  Caution: The string passed to this
 * must be at least 12 bytes long
 *------------------------------------------------------------------------
 */
char *itoa(uint value, char *buf, uchar radix)
{
	unsigned int 	i;
	char 		*ptr;
	char 		*temphold;
	
	temphold = buf;
	ptr = buf + 12;
	*--ptr = 0;		/* Insert NULL char */
	do {
		/* First create string in reverse order */
		i = (value % radix) + 0x30;
		if(i > 0x39) i += 7;
		*--ptr = i;
		value = value / radix;
	} while(value != 0);
	
	/* Next, move the string 6 places to the left
	 * Include NULL character
	 */
	for( ; (*buf++ = *ptr++); );

	return(temphold);
}

int atoi(char *string)
{
	int length;
	int retval = 0;
	int i;
	int sign = 1;

	length = strlen(string);
	for (i = 0; i < length; i++) {
		retval *= 16;
		if (0 == i && string[0] == '-') {
			sign = -1;
			continue;
		} else if (string[i] <= '9' && string[i] >= '0')
			retval += string[i] - '0';
		else if (string[i] <= 'F' && string[i] >= 'A')
			retval += string[i] - 'A'+10;
		else if (string[i] <= 'f' && string[i] >= 'a')
		  	retval += string[i] - 'a'+10;
		else
                  	break;
	}
	retval *= sign;
	return retval;
}
#endif


uint http_header(char *pBuf, int nStatus, char *szTitle, int nLength)
{
	unsigned int	index = 0;
	
	index += sprintf(&pBuf[index], "%s %d %s\r\n", _HTTP_PROTOCOL, nStatus, szTitle);
	index += sprintf(&pBuf[index], "Server: %s\r\n", _HTTP_SERVER);
	index += sprintf(&pBuf[index], "Connection: close\r\n");
	index += sprintf(&pBuf[index], "Content-Type: text/html\r\n");
	if (nLength > 0)
		index += sprintf(&pBuf[index], "Content-Length: %d\r\n", nLength);
	index += sprintf(&pBuf[index], "\r\n");
	
	return index;
}

uint http_send_ok(uchar nr, char *html_page, int nStatus, char *szTitle)
{
	char	*pbuf;
	char	html_buf[MAX_ETHER_PKT_LEN];
	uint	page_len = 0;
	uint 	hdr_len;
	uint 	body_len;

	/* fill html title to html buffer */
	page_len += sprintf(&html_buf[page_len], 
			"<html><head><title>%s</title></head>", _HTTP_TITLE);
	/* copy html body to html buffer */
	memcpy(&html_buf[page_len], html_page, strlen(html_page));
	page_len += strlen(html_page);
	
	/* fill html header to packet buffer */
	pbuf = http_buf + 54;
	hdr_len = http_header(pbuf, nStatus, szTitle, page_len);
	
	/* copy html buffer to packet buffer */
	memcpy(&pbuf[hdr_len],html_buf,page_len);
	
	body_len = hdr_len + page_len;
	
	pbuf[54 + body_len] = 0;	/* Append NULL */

	/* Segment length = body_len + 20(TCP header) */
	http_send((uchar *)http_buf, body_len + 20, nr);
	conxn[nr].my_sequence += body_len;
	
	return 0;	
}

uint http_send_error(uchar nr, int nStatus, char *szTitle, char *szText)
{
	char	*pbuf;
	char	html_buf[MAX_ETHER_PKT_LEN];
	uint	page_len = 0;
	uint 	hdr_len;
	uint 	body_len;
	
	page_len += sprintf(&html_buf[page_len], 
			"<html><head><title>%s</title></head>", _HTTP_TITLE);
	page_len += sprintf(&html_buf[page_len],
			"<body>%s</body></html>", szText);
			
	pbuf = http_buf + 54;
	hdr_len = http_header(pbuf, nStatus, szTitle, page_len);
	memcpy(&pbuf[hdr_len],html_buf,page_len);
	
	body_len = hdr_len + page_len;
	
	pbuf[54 + body_len] = 0;	/* Append NULL */

	/* Segment length = body_len + 20(TCP header) */
	http_send((uchar *)http_buf, body_len + 20, nr);
	conxn[nr].my_sequence += body_len;
	
	return 0;
}

/*------------------------------------------------------------------------
 * This sends an TCP segment to the ip layer.  The segment is 
 * is normally either a web page or a graphic.
 * See "TCP/IP Illustrated, Volume 1" Sect 17.3
 *------------------------------------------------------------------------
 */
void http_send(uchar *outbuf, uint len, uchar nr)
{
	TCP_t 	*tcp;
	IP_t 	*ip;
	uint 	sum;
	ushort 	result;
	uint	dest_ip;
	  
	/* Fill in TCP segment header */
	tcp = (TCP_t *)(outbuf + 34);
	ip = (IP_t *)(outbuf + 14);
	
	tcp->source_port = htons(HTTP_PORT);
	tcp->dest_port = htons(conxn[nr].port);
	tcp->sequence = htonl(conxn[nr].my_sequence);
	tcp->ack_number = htonl(conxn[nr].his_sequence);
	
	/* Header is always 20 bytes long */
	tcp->flags = htons(0x5000 | FLG_ACK | FLG_PSH);
	tcp->window = htons(WINDOW_SIZE);
	tcp->checksum = 0;
	tcp->urgent_ptr = 0;

	/* 
	 * Compute checksum including 12 bytes of pseudoheader
	 * Must pre-fill 2 items in ip header to do this
	 */
	dest_ip = htonl(conxn[nr].ipaddr);
	NetCopyIP((void*)&ip->ip_src, &NetOurIP); /* in network byte order */
	NetCopyIP((void*)&ip->ip_dst, &dest_ip);
	
	/* Sum source_ipaddr, dest_ipaddr, and entire TCP message */
	sum = (uint)cksum(outbuf + 26, 8 + len);
			
	/*
	 * Add in the rest of pseudoheader which is
	 * protocol id and TCP segment length
	 */
	sum += (uint)htons(0x0006);
	sum += (uint)htons(len);
	
	/* In case there was a carry, add it back around */
	result = (uint)(sum + (sum >> 16));
	tcp->checksum = ~result;

	memcpy(outbuf, conxn[nr].mac, 6);
   
	if (http_debug == DEBUG)
		printf("%s : Sending msg to IP layer\n", __func__);
	NetSendIPPacket(outbuf, dest_ip, TCP_TYPE, len);

	/* (Re)start TCP retransmit timer */
	conxn[nr].timer = TCP_TIMEOUT;
}


/*------------------------------------------------------------------------
 * This searches a web page looking for a specified tag.  If found,
 * it replaces the tag with the text in * sub.  Tags are fixed length -
 * The first 4 chars of the tag is always "TAG:" and the rest of it
 * is always 4 chars for a total of 8 chars. 
 *------------------------------------------------------------------------
 */
void replace_tag(uchar *start, char *tag, char *sub) 
{ 
	uchar i, flg;
	uchar *ptr;
	
	/* Find tag.  If not found - just return */
	ptr = (uchar *)strstr((const char *)start, tag);
	if (ptr == NULL) return;
	
	flg = TRUE;
	
	/* Replace the 8 char tag with the substitute text
	 * Pad on the right with spaces
	 */
	for (i=0; i < 8; i++) {
		if (sub[i] == 0) 
			flg = FALSE;
			
		if (flg) 
			ptr[i] = sub[i]; 
		else 
			ptr[i] = ' ';
	}
}

static int http_check_filename(uchar *tcp_data)
{
	uchar 	*ptr;
	
	if ((ptr = (uchar *)strstr((const char *)tcp_data,
		 boundary_code)) != NULL) {
		/* move pointer to Content-Disposition */
		ptr = ptr + strlen(boundary_code);

		/* get upload file name */
		ptr = (uchar *)strstr((const char *)ptr, c_fname); 
		ptr = (uchar *)strchr((const char *)ptr,'"');
		if (*(ptr+1) == '"') {
			printf("No file to be uploaded !!!\n");
			return 1;
		} else {
			printf("Parsing file name...\n");
		}
								
	} else {
		printf("%s : Can't find boundary code !!!\n",__func__);
		return 1;
	}

	return 0;	
}

static int http_parse_mime(uchar *tcp_data, ushort tcp_data_len, uchar nr)
{
	uchar 	*ptr;
	uint	data_len;
	uint	mem_len;

	/* skip boundary="boundary code" */
	if ((ptr = (uchar *)strstr((const char *)tcp_data, 
			boundary_code)) == NULL)
		return 1;
	else 
		ptr = ptr + strlen(boundary_code);

	mem_len = tcp_data_len - (ptr - tcp_data);
	
	/* find the first boundary code */
	if ((ptr = (uchar *)memstr((const char *)ptr, 
			boundary_code, mem_len)) != NULL) {
		/* the first content length */
		post_info.rcv_len = tcp_data_len - (ptr - tcp_data) + 2;
		
		/* move pointer to Content-Disposition */
		ptr = ptr + strlen(boundary_code);

		/* get upload file name */
		ptr = (uchar *)strstr((const char *)ptr, c_fname);
		ptr = (uchar *)strchr((const char *)ptr,'"');
		if (*(ptr+1) == '"') {
			printf("No file to be uploaded !!!\n");
			http_state = HTTP_START;
			http_send_error(nr, 400, "Bad Request",
					 "No upload file name to be slected !");
			return 1;
		} else {
			/* printf("Parsing file name...\n"); */
		}
								
	} else {
		/*printf("Can't find the first boundary code.\n");*/
		return 1;
	}
	
	ptr = (uchar *)strstr((const char *)ptr, c_headerend); /* "\r\n\r\n" */
	ptr = ptr + strlen(c_headerend);	/* upload file start pointer */
	
	data_len = tcp_data_len - (ptr - tcp_data );
	post_info.data_len = data_len;
	memcpy(post_info.buf, ptr, data_len);
	
	return 0;
}


static int http_parse_post(uchar *tcp_data, uint *content_length)
{
	uchar 	*ptr;
	
	/* try to retrieve the "content length" from HTTP packet */
	if ((ptr = (uchar *)strstr((const char *)tcp_data, c_len)) != NULL) {
		ptr = ptr + strlen(c_len);
		*content_length = simple_strtoul((const char *)ptr,NULL,10);
		if (http_debug == DEBUG)
			printf("content_length = %d\n",*content_length);
			
		
	} else {
		printf("%s:Can't retrieve content length.\n",__func__);
		return 1;
	}

	/* try to retrieve the "boundary code" from HTTP packet */
	if ((ptr = (uchar *)strstr((const char *)tcp_data, c_boundary))
		 != NULL) {
		ptr = ptr + strlen(c_boundary);
		ptr = (uchar *)strtok((char *)ptr,"\r\n");
		memset(&boundary_code[0],0x00,sizeof(boundary_code));
		memcpy(&boundary_code[0], ptr, strlen((const char *)ptr));
		if (http_debug == DEBUG)
			printf("%d >>> boundary_code = %s\n",
				strlen(boundary_code),boundary_code);
	} else {
		printf("%s : Can't retrieve boundary code.\n",__func__);
		return 1;
	}
	
	return 0;
}


/*------------------------------------------------------------------------
 * The received header must contain the word "GET" or "POST" to be 
 * considered a valid request.
 * With HTTP 1.1 where the connection is left open, the header I send
 * should include content length. With HTTP 1.0 you can just close the
 * connection after sending the page and the browser knows its done. 
 *
 * The HTTP protocol specification is at http://www.w3.org/Protocols/ 
 *------------------------------------------------------------------------
 */
ushort http_server(uchar *inbuf, ushort header_len, ushort tcp_len,
		   uchar nr, uchar resend)
{
	static uint	counter=0;
	uint	content_length;
	uint	data_len;
	uchar 	*ptr;
	uchar 	*tcp_data;

	if (http_debug == DEBUG)
		printf("\n\n%s************ tcp_len = %d ************%d\n",
			__func__,tcp_len,http_state);
	else
		if ((counter++ % 100) == 0)
			printf(".");
		    	
	/* Make sure this is a valid connection */
	if (nr == NO_CONNECTION) 
		return 0;
	
	/* Compute start of TCP data */
	
	/* Save first 20 chars and seq number just in case
	 * we need to re-generate page
	 * TODO: if post, then save switch state infomation
	 * If this is a resend, set sequence number to what it was
	 * the last time we sent this
	 */
	if (!resend) {
		tcp_data = inbuf + ETHER_IP_HLEN + header_len;
		memcpy(conxn[nr].query, tcp_data, 20);
		conxn[nr].old_sequence = conxn[nr].my_sequence;
	} else {
		tcp_data = inbuf;
		conxn[nr].my_sequence = conxn[nr].old_sequence;   
	}

	if (http_debug == DEBUG)
		printf("#1> http_state = %d......\n",http_state);

	/* Pre-porcess HTTP state change. */
	switch (http_state) {
	case HTTP_START:
		if ( (strstr((const char *)tcp_data, "GET") != NULL) && 
		     ((strstr((const char *)tcp_data, "index") != NULL) || 
		      (strstr((const char *)tcp_data, "/ ") != NULL)) )
			http_state = HTTP_GET;
		else if ( (strstr((const char *)tcp_data, "POST") != NULL) )
			http_state = HTTP_POST;
		break;

	case HTTP_GET:
		if ( (strstr((const char *)tcp_data, "POST") != NULL) )
			http_state = HTTP_POST;
		break;
		
	case HTTP_UPLOAD:  /* process the situation of network disconnection */
		if ( (strstr((const char *)tcp_data, "GET") != NULL) && 
		     ((strstr((const char *)tcp_data, "index") != NULL) || 
		      (strstr((const char *)tcp_data, "/ ") != NULL)) )
			http_state = HTTP_GET;
		break;
	}
	
	switch (http_state) {
	case HTTP_START:
		break;
		
	case HTTP_GET:
	      	/* send download page to HTTP client */
		http_send_ok(nr, web_page, 200, "OK");
		break;

	case HTTP_POST:
		if (http_parse_post(tcp_data, &content_length)) {
			http_state = HTTP_START;
			return 1;
		} else {
			post_info.upload_len = content_length;
			post_info.data_len = 0;
			post_info.buf = (uchar *)upgrade_buffer;
			http_state = HTTP_HEAD_DATA;
		}

		if (http_parse_mime(tcp_data, tcp_len - header_len, nr) == 0)
			http_state = HTTP_UPLOAD;
		break;

	case HTTP_HEAD_DATA:
		post_info.rcv_len = tcp_len - header_len;

		if (http_check_filename(tcp_data)) {
			http_send_error(nr, 400, "Bad Request",
				"No upload file name to be slected !");
			http_state = HTTP_START;
			return 1;			
		}
		
		ptr = (uchar *)strstr((const char *)tcp_data, c_headerend);
		ptr = ptr + strlen(c_headerend); /* upload file start pointer */
		
		data_len = tcp_len - header_len - (ptr - tcp_data );
		post_info.data_len = data_len;
		memcpy(post_info.buf, ptr, data_len);

		http_state = HTTP_UPLOAD;
		break;

	case HTTP_UPLOAD:
		post_info.rcv_len += tcp_len - header_len;

		if (http_debug == DEBUG) {
			printf("post_info.rcv_len=%d  post_info.upload_len=%d\n"
				,post_info.rcv_len,post_info.upload_len);
		}
		
		if (post_info.rcv_len >= post_info.upload_len) {
			/* To check the last packet if contains image data. */
			/* 6-byte include two "--" and one \r\n */
			/* 8-byte include two "--" and two \r\n */
			if ((tcp_len - header_len) > (strlen(boundary_code)+6)) 
				data_len = tcp_len - header_len - \
						strlen(boundary_code) - 8;
			else
				data_len = 0;

			if (http_check_image(post_info.buf, post_info.data_len)) {
				http_send_error(nr, 400, "Bad Request",
						 "File format is invalid !");
				http_state = HTTP_START;
				return (1);
			} else {			
				memset(text, 0x00, sizeof(text));
#ifdef CONFIG_MTD_CORTINA_CS752X_NAND
				sprintf(text,"%d", 5 + post_info.data_len/(1024*1000));
#else
				sprintf(text,"%d", 10 + post_info.data_len/(1024*100));
#endif
				replace_tag((uchar *)upgrade200_1, "TAG:NUM1", text);
	
				http_send_ok(nr, upgrade200_1, 200, "OK");	

				if (do_upgrade_image(post_info.buf, post_info.data_len)) {
					http_state = HTTP_START;
					return 0;
				} else {
					http_state = HTTP_REBOOT;
					break;
				}
			}

		} else {
			data_len = tcp_len - header_len;
			memcpy((void *)&post_info.buf[post_info.data_len], tcp_data, data_len);
			post_info.data_len += data_len;
		}
			
		break;

	case HTTP_REBOOT:
		if ( (strstr((const char *)tcp_data, "GET") != NULL) && 
		     (strstr((const char *)tcp_data, "reboot") != NULL) ) {    	
			http_state = HTTP_START;
			http_send_ok(nr, reboot200, 200, "OK");
			/* 
			 * Reserved time for tx those packets that queue 
			 * in packet buffer before system reset.
			 */
			udelay(500);
			do_reset(NULL, 0, 0, NULL);
		}
		break;

	default:
		printf("HTTP State Error ! %d\n", http_state);
		break;
	}

	if (http_debug == DEBUG)
		printf("#2> http_state = %d......\n",http_state);
	
	return 0;
}

int do_http (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int	ret;

	printf("HTTP Start......\n");
	
	if (argc > 1)
		http_debug = DEBUG;
	else
		http_debug = 0;

	http_state = HTTP_START;

	init_tcp();

	if (ret = NetLoop(HTTP) < 0) {
		printf("HTTP Stop......%d\n",ret);
		return 1;
	}

	printf("HTTP Stop......%d\n",ret);
	
	return 0;
}

U_BOOT_CMD(
	http,	2,	1,	do_http,
	"Start HTTP server",
	"http"
);

