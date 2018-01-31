#include <stdio.h>
#include <alloc.h>

__ATTR_LIB_C__ short fclose(FILE *f)
{
  short s;
  
  if(!f) return EOF;
  s=(f->flags&_F_ERR)?EOF:0;
  
  HeapRealloc(f->handle,(*(unsigned short*)(f->base))+2);
  HeapUnlock(f->handle);
  free(f);
  
  return s;
}
