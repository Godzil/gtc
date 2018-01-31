#ifdef USE_ABSOLUTE_PATHS
#include "E:\Paul\89\Ti-GCC\Projects\GT-Dev\SecureCommDef.h"
#else
#include "securecommdef.h"
#endif

#ifndef PEDROM
SecureTab *GetAppSecureTable(char *name) {
	int appid=EV_getAppID(name);
	void *tmp;
	AppHdr *hdr;
	if (appid<=0) return NULL;
	tmp=HeapDeref(appid);
	(const AppHdr *)hdr=((ACB *)tmp)->appHeader;
	if (hdr->magic!=OO_APP_MAGIC || strcmp(hdr->name,name))
		return NULL;
	return *(SecureTab **)(((void *)hdr)+hdr->codeOffset);
}
#else
SecureTab *GetAppSecureTable(char *name) {
	long *ptr=ROM_base+0x30000;
	#define MK_TAG2(a,b) (((long)(a)<<8)+(b))
	#define MK_TAG(a,b,c,d) MK_TAG2(MK_TAG2(MK_TAG2(a,b),c),d)
	if (*ptr++!=MK_TAG('G','T','C','.') || *ptr++!=MK_TAG('t','a','g',' '))
		return NULL;
	return *(SecureTab **)ptr;
}
#endif
