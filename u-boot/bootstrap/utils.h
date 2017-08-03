#ifndef __UTILS_H__
#define __UTILS_H__

#define REG_READ_UINT32( _register_, _value_ ) \
        ((_value_) = *((volatile unsigned int *)(_register_)))

#define REG_WRITE_UINT32( _register_, _value_ ) \
        (*((volatile unsigned int *)(_register_)) = (_value_))

#define REG_WRITE_UINT16( _register_, _value_ ) \
        (*((volatile unsigned short *)(_register_)) = (_value_))

#define REG_WRITE_UINT8( _register_, _value_ ) \
        (*((volatile unsigned char *)(_register_)) = (_value_))

inline int islower( int c );
inline int tolower( int c );

extern unsigned int iros_strtol( const char *nptr);
extern int memcpy(void *dest, void *srce, int size);
extern void memset(void *buf,int val, int len);
extern int memcmp(void *buf,void *buf1, int len); 
extern void * memmove(void * dest,const void *src,int count);
extern int _rb_gets(char *buf, int buflen, int timeout);
extern void serial_put_hex(unsigned int val);
extern int post_strlen(char *s1);

#endif
