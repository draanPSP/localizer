#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "utils.h"

void MD5Hash(void *buf, int size, md5_byte_t *out)
{
     md5_state_t state;
     md5_init(&state);
	 md5_append(&state, (const md5_byte_t *)buf, size);
	 md5_finish(&state, out);
}

char* utftochar(char* string, int len)
{
      char* output = (char*)malloc(len/2 + 1);
      memset(output, 0, len/2);
      int i;
      for(i = 0; i < len/2; i++)
      {
            output[i] = string[i*2];
      }
      output[len/2] = 0;
      return output;
}

int getLen(wchar_t *buf)
{
    int size = 0;
    while(1)
    {
            if(buf[size] == 0xD && buf[size+1] == 0xA) break;
            size++;
    }
    return size;
}

void wstrncpy(wchar_t *dest, wchar_t *src, int size)
{
     int i = 0;
     while(src[i] != 0 || i < size)
     {
          dest[i] = src[i];
          i++;
     }
}

int wstrlen(wchar_t *str)
{
    int i = 0;
    while(str[i] != 0)
        i++;
    return i;
}

const char HEXstring[17] = "0123456789ABCDEF";

char checkOneHex(char nibble)
{
     return HEXstring[nibble & 0x0F];
}

/*
char checkOneHex(char nibble)
{
   char nib = nibble & 0x0F;
   if (nib > 10) nib = nib + 'A' - ('9' + 1);
   return (nib + '0');
}
*/

