#include <stdio.h>
#include <tigcclib.h>

__ATTR_LIB_C__ FILE *fopen(const char *name, const char *mode)
{
  char str[50],*fpos,*epos,*base=0,*sptr=str,chmode=mode[0];
  short bmode=(mode[1]=='b'|| mode[2]=='b'),flags=0,ferr;
  SYM_ENTRY *sym_entry;
  FILE *f;
  
  *sptr=0; while((*++sptr=*name++));
  
  if((sym_entry=DerefSym(SymFind(sptr))))
    if(sym_entry->flags.flags_n&0x8218 && strpbrk(mode,"wa+"))
      return 0;
  
  if(!(f=malloc(sizeof(FILE)))) return 0;
  
  if(chmode=='r'|| chmode=='a')
    {
      flags=_F_READ;
      if(!sym_entry)
        if(chmode=='r')
          {
            free(f);
            return 0;
          }
        else chmode='w';
      else
        {
          base=HLock(f->handle=sym_entry->handle);
          f->alloc=HeapSize(f->handle);
        }
    }
  if(chmode=='w')
    {
      SCR_STATE scr_state;
      flags=_F_WRIT;
      SaveScrState(&scr_state);
      ferr=!(sym_entry=DerefSym(SymAdd(sptr)));
      RestoreScrState(&scr_state);
      if(!ferr) ferr=!(f->handle=sym_entry->handle=HeapAlloc(128));
      if(ferr)
        {
          SymDel(sptr);
          free(f);
          return 0;
        }
      
      f->buffincrement=f->alloc=128;
      
      base=HLock(f->handle);
      if(bmode) poke_w(base,0);
      else
        {
          poke_l(base,0x00050001);
          poke_l(base+4,0x2000E000);
        }
    }
  epos=base+peek_w(base)+(bmode?2:0);
  if(chmode=='a') flags=_F_WRIT,fpos=epos;
  else fpos=base+(bmode?2:5);
  if(epos==fpos) flags|=_F_EOF;
  if(mode[1]=='+'||mode[2]=='+') flags|=_F_RDWR;
  if(bmode) flags|=_F_BIN;
  f->flags=flags; f->base=base; f->fpos=fpos; f->unget=0;
  return f;
}
