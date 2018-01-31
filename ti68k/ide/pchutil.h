// PCH Utils file

#define PU_HANDLER(f) char *PchRCB_##f \
	(char *name,char *args,char *defn,char *proto,char *trueproto,char *param)

#define PchUnpack(f,p,p0) (pch_read(p,p0,PchRCB_##f,NULL))
#define PchUnpackP(f,p,p0,cbp) (pch_read(p,p0,PchRCB_##f,cbp))

short __attribute__((stkparm)) sUnpackSize(char *in,char *dic);

typedef char *(*PCHREAD_CB)(char *name,char *args,char *defn,char *proto,char *trueproto,char *param);
char *write_until(char *d,char **s,char c) {
	char *p=*s;
	while (*p && *p!=c)
		*d++=*p++;
	*s=p+1;
	return d;
}
char *pch_read(char *p,char *p0,PCHREAD_CB cb,char *cbparam) {
	char *name=p,*args0=NULL,*defn0,*proto0;
	char *args,*defn,*proto,*pseudoproto;
	int flags;
	while (*p++);
	p+=2*3;
	if ((flags=((char *)p)[-2]<<8)&PCHID_MACRO) {
		args0=p;
		while (*p)
			while (*p++);
		p++;
	}
	defn0=p;
	while (*p++);
	proto0=p;
	p0+=((PCH_HEAD *)p0)->dic_off;
	proto=alloca(sUnpackSize(proto0,p0));
	sUnpack(proto0,proto,p0);
	pseudoproto=proto;
	if (args0) {
		char *d;
		args=d=alloca(defn0-args0);
		*d++='(';
		p=args0;
		if (*p)
			while (1) {
				while ((*d++=*p++));
				d--;
				if (!*p) break;
				*d++=',';
			}
		*d++=')';
		*d=0;
	} else args="";
	if ((flags&PCHID_PACKED)) {
		defn=alloca(sUnpackSize(defn0,p0));
		sUnpack(defn0,defn,p0);
	} else defn=defn0;
	if (*defn=='_' && !strncmp(defn,"_rom_call(",10)) {
		char *s=defn+10,*d=alloca(strlen(s)+2);
		pseudoproto=d;
		d=write_until(d,&s,',');
		if (d[-1]!='*') *d++=' ';
		*d++='_';
		d=write_until(d,&s,')');
		*d++=')';
		*d++=';';
		*d=0;
	}
	return cb(name,args,defn,pseudoproto,proto,cbparam);
}

asm("
	xdef sUnpackSize
sUnpackSize:
	move.l 4(%sp),%a0
	moveq #1,%d0
	moveq #126,%d1
	moveq #0,%d2
	move.b (%a0)+,%d2
	beq sus_quit
	bmi sus_special
sus_copy_lp:
	addq.w #1,%d0
sus_next:
	move.b (%a0)+,%d2
	bgt sus_copy_lp
	beq sus_quit
sus_special:
	subq.b #1,%d2
	bmi sus_not_escape
	move.b (%a0)+,%d2
	bra sus_copy_lp
sus_not_escape:
	addq.b #2,%d2
	bmi sus_not_romcall
	
sus_not_romcall:
	add.b %d1,%d2
	move.l 8(%sp),%a1
	dbf %d2,sus_search_loop
	bra sus_search_done
sus_search_loop:
	tst.b (%a1)+
	bne sus_search_loop
	dbf %d2,sus_search_loop
sus_search_done:
	addq.w #1,%d0
	move.b (%a1)+,%d2
	bne sus_search_done
	subq.w #1,%d0
	moveq #0,%d2
	bra sus_next
sus_quit:
	rts");
