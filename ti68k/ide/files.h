// Files misc mgt routines

// internal filetypes...
enum {
	IFT_INVALID=0,
	IFT_OTH, IFT_TEXT, IFT_RES, IFT_C, IFT_H, IFT_PRGM, IFT_FUNC, IFT_XFUNC,
	IFT_PACK /* (?) */, IFT_EXPR,
};
// remark : currently IFT_XFUNC and IFT_PACK are never generated...
int IFTIcons[]={
	[IFT_INVALID] 0,
	[IFT_OTH]  3,
	[IFT_TEXT] 4,
	[IFT_RES]  5,
	[IFT_C]    6,
	[IFT_H]    7,
	[IFT_PRGM] 8,
	[IFT_FUNC] 8,
	[IFT_XFUNC]8,
	[IFT_PACK] 9,
	[IFT_EXPR] 10,
};

HANDLE FindFile(char *f) {
	HSym hs;
	if (!(hs=SymFind(SYMSTR(f))).folder)
		return H_NULL;
	SYM_ENTRY *se=DerefSym(hs);
	return se->handle;
}
int GetIFT(HANDLE h) {
	ESI tagptr=HToESI(h);
	switch (*tagptr) {
		case OTH_TAG:
			tagptr--;
			while (*--tagptr);
			tagptr++;
			switch (*tagptr) {
				case 'C':
					if (!strcmp(tagptr,"C")) return IFT_C;
					break;
				case 'H':
					if (!strcmp(tagptr,"H")) return IFT_H;
					break;
				case 'R':
					if (!strcmp(tagptr,"RES")) return IFT_RES;
					break;
			}
			return IFT_OTH;
		case TEXT_TAG:
			return IFT_TEXT;
		case USERFUNC_TAG:
			return IFT_PRGM;
	}
	return IFT_EXPR;
}
char *GetFTString(HANDLE h) {
#ifndef PEDROM
	ESI tagptr=HToESI(h);
	if (*tagptr==OTH_TAG) {
		tagptr--;
		while (*--tagptr);
		tagptr++;
		return tagptr;
	}
	return DataTypeNames(*tagptr);
#else
	return "???";
#endif
}

XP_C *XpLoadItems(int filter) {
	WITEM *wi=wsp->items;
	int n=wsp->nitems;
	XP_C *xc=malloc(sizeof(XP_C)+(n+2)*sizeof(XP_S));
	XP_S *xs;
	if (!xc) return xc;
	xs=(XP_S *)(xc+1);
	xs->i=0; xs++; xc->s=xs;
	xc->sh=0; xc->sel=0; xc->msel=n-1;
	while (n--) {
		if (wi->d.type==WI_FOLD) {
			*(long *)&xs->i=0x00010002;
			xs->d=wi->d.title;
		} else {
			int t=GetIFT(FindFile(wi->f.name));
			if (filter&(1<<t)) goto skipit;
			xs->i=0xC002;
			xs->t=IFTIcons[t];
			xs->d=wi->f.name;
		}
		xs++;
	skipit:
		wi++;
	}
	xs->i=0;
	return xc;
}
