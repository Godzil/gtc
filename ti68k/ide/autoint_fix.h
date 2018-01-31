#ifndef OSKeyScan
#define OSKeyScan _rom_call(void,(void),298)
#endif

#ifndef OLD_NONTITANIUM_METHOD
void *autoint_fix=0,*autoint_previous=0;
#define MAX_BLOCK1 150
#define MAX_BLOCK2 500
int autoint_fix_block[(MAX_BLOCK1+MAX_BLOCK2)/2];
// !!! DIRTY HACK !!!
//  -- but speeds up a whole lot :)
// !!! CAUTION : not tested on AMS3 !!!
void FixAutoint() {
#ifndef PEDROM
	autoint_fix=0;
	void *v1=GetIntVec(AUTO_INT_1);
	autoint_previous=v1;
	if (v1>=(void *)0x200000l) {
		void *v2=(void *)OSKeyScan;
		int l2=(void *)off - v2;
		if ((unsigned int)l2>=MAX_BLOCK2) return;
		int pos=0;
		do {
			pos+=2;
			if (*(int *)(v1+pos)==0x4e75 || *(int *)(v1+pos)==0x4e73) return;
		} while (*(void **)(v1+pos)!=v2);
		pos+=4+6;	// leave some room for the jmp
		if (pos>=MAX_BLOCK1) return;
		autoint_fix=autoint_fix_block;
		memcpy(autoint_fix,v1,pos);
		memcpy(autoint_fix+pos,v2,l2);
		*(void **)(autoint_fix+pos-10)=autoint_fix+pos;
		*(int *)(autoint_fix+pos-6)=0x4ef9;	// jmp abs.l
		*(void **)(autoint_fix+pos-4)=v1+pos-6;
		int *z=autoint_fix+pos;
		while (*z++!=0x58 && l2--);
		if (l2>0) *--z=0x10;
		SetIntVec(AUTO_INT_1,autoint_fix);
	}
#endif
}
void RestoreAutoint() {
#ifndef PEDROM
	if (autoint_fix)
		SetIntVec(AUTO_INT_1,autoint_previous),autoint_fix=0;
#endif
}
#else
#define IntVec (*(void **)0x40064l)
void *autoint_fix=0,*autoint_previous=0;
// !!! DIRTY HACK !!!
//  -- but speeds up a whole lot :)
// !!! CAUTION : not Titanium-compliant !!!
void FixAutoint() {
#ifndef PEDROM
	autoint_fix=0;
	void *v1=IntVec;
	autoint_previous=v1;
	if (v1>=(void *)0x200000l) {
		void *v2=(void *)OSKeyScan;
		int l2=(void *)off - v2;
		if ((unsigned int)l2>=500) return;
		int pos=0;
		do {
			pos+=2;
			if (*(int *)(v1+pos)==0x4e75) return;
		} while (*(void **)(v1+pos)!=v2);
		pos+=4+6;	// leave some room for the jmp
		if (pos>=150) return;
		autoint_fix=malloc(pos+l2);
		if (!autoint_fix) return;
		autoint_fix+=0x40000;	// switch to ghost space
		memcpy(autoint_fix,v1,pos);
		memcpy(autoint_fix+pos,v2,l2);
		*(void **)(autoint_fix+pos-10)=autoint_fix+pos;
		*(int *)(autoint_fix+pos-6)=0x4ef9;	// jmp abs.l
		*(void **)(autoint_fix+pos-4)=v1+pos-6;
		int *z=autoint_fix+pos;
		while (*z++!=0x58 && l2--);
		if (l2>0) *--z=0x10;
		IntVec=autoint_fix;
	}
#endif
}
void RestoreAutoint() {
#ifndef PEDROM
	if (autoint_fix)
		IntVec=autoint_previous,free(autoint_fix);
#endif
}
#undef IntVec
#endif
