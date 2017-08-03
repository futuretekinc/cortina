//-----------------------------------------------------------------------------
// TCP.H
//
//-----------------------------------------------------------------------------
#ifndef __TCP_H__
#define __TCP_H__

// TCP states
#define STATE_CLOSED				0
#define STATE_LISTEN				1
#define STATE_SYN_RCVD				2
#define STATE_ESTABLISHED			3
#define STATE_CLOSE_WAIT			4
#define STATE_LAST_ACK				5
#define STATE_FIN_WAIT_1			6
#define STATE_FIN_WAIT_2			7
#define STATE_CLOSING				8
#define STATE_TIME_WAIT				9

#ifndef FALSE
#define FALSE					0
#endif

#ifndef TRUE
#define TRUE					1
#endif

#define HTTP_PORT  				80
#define TCP_TYPE              			6

// TCP flag bits
#define FLG_FIN					0x0001
#define FLG_SYN					0x0002
#define FLG_RST					0x0004
#define FLG_PSH					0x0008
#define FLG_ACK					0x0010
#define FLG_URG					0x0020

#define WINDOW_SIZE				(1024)
// Miscellaneous
#define NO_CONNECTION  				5 
#define TCP_TIMEOUT				4	// = 2 seconds
#define INACTIVITY_TIME				30	// = 15 seconds

typedef struct
{
	unsigned int 	ipaddr;
	unsigned int  	his_sequence;
	unsigned int  	my_sequence;
	unsigned int  	old_sequence;
	unsigned int  	his_ack;
	unsigned short  port;
	unsigned char 	timer;
	unsigned char 	inactivity;	 
	unsigned char 	state;
	unsigned char 	resv[1];
	unsigned char	mac[6];
	unsigned char 	query[20];
} TCP_CONNECTION;

typedef struct
{
	unsigned short  source_port;
	unsigned short  dest_port;
	unsigned int  	sequence;
	unsigned int  	ack_number;
	unsigned short  flags;
	unsigned short  window;
	unsigned short  checksum;
	unsigned short  urgent_ptr;
	unsigned char 	options[8];
} __attribute__ ((aligned(1), packed)) TCP_t;

void tcp_send(ushort, ushort, uchar);
void tcp_rcve(uchar *, uint);
void tcp_retransmit(void);
void tcp_inactivity(void);
void init_tcp(void);
ushort cksum(uchar *check, ushort length);

extern int NetSendIPPacket(uchar *ether, IPaddr_t dest, uchar proto_id, uint len);

#endif /*__TCP_H__*/


