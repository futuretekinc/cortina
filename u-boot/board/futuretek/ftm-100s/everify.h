#ifndef __EVERIFY_H__
#define __EVERIFY_H__

#include "../../../lib_openssl/ossl-api.h"

//OSSLAPI*		osslapi;

int do_verify(unsigned char *addr, unsigned int len);
void get_element(unsigned char type, unsigned char *buf, unsigned int *len);
int check_blank(unsigned char *ptr);

#endif //__EVERIFY_H__
