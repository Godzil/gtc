#include <stdlib.h>
#include <ctype.h>

#ifndef CONFIRM_REGPARM1_ENABLED
#error You need to patch <default.h> as described in the 'COMPILING' file.
#endif

__ATTR_LIB_C__ short atoi(const char *s)
{
  unsigned short c,value=0,neg=0;
  while(*s==' ') ++s;
  if(*s=='-'||*(const unsigned char*)s==0xAD) ++s,neg=-1;
  else if(*s=='+') ++s;
  while(isdigit(c=*(const unsigned char*)s++)) value=10*value+c-'0';
  return neg?-value:value;
}
