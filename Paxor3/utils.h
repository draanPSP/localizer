#ifndef UTILS
#define UTILS

#include "md5.h"

void MD5Hash(void *buf, int size, md5_byte_t *out);

char* utftochar(char* string, int len);

int getLen(wchar_t *buf);

void wstrncpy(wchar_t *dest, wchar_t *src, int size);

int wstrlen(wchar_t *str);

char checkOneHex(char nibble);
#endif

