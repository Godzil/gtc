/*
 * GTools C compiler
 * =================
 * source file :
 * sPack header packing/unpacking
 */

#include	"define.h"

#define SPK_START (char)0x81
#ifdef STATIC_SPACK_TAB
#ifdef ARRAY_VER
#define C(al) al
char *sPk_tab[]={
#else
#define C(al) al "\0"
char *sPk_table=
	"\0"
#endif
	C(" *)")
	C("*)")
	C("*,")
	C("int x")
	C("int y")
	C("int z")
	C("int a")
	C("int b")
	C("int c")
	C("register ")
	C("unsigned ")
	C("signed ")
	C("volatile ")
	C("void _(")
	C("int _(")
	C("short _(")
	C("BOOL _(")
	C("long _(")
	C(" *_(")
	C("*_(")
	C("const ")
	C("void,(")
	C("char,(")
	C("int,(")
	C("short,(")
	C("long,(")
	C("float,(")
	C("void")
	C("char")
	C("int")
	C("short")
	C("long")
	C("float")
	C("typedef struct")
	C("typedef ")
#ifdef GEN
	C("FIXED ")
	C("HANDLE ")
	C("BGS *")
	C("SCREEN *")
	C("POINT *")
	C("FIXED *")
	C("PLANE *p")
#endif
	C("Gray")
	C("_RR(")
	C(",15")
	C(",(")
	C("),")
	C("({")
	C("})")
	C("_rom_call(")
	C("_rom_call_addr(")
	C("FILE*")
	C("__ATTR_LIB_ASM__")
	C("__ATTR_LIB_C__")
	C("__ATTR_TIOS__")
	C("__ATTR_")
	C(" __")
	C("__")
	C(" *")
#ifdef ARRAY_VER
	C(0)
};
#else
	C("")
		- 1
;
#endif
#else
char *sPk_table=0;
#endif
#undef C
/*#define sPk_num sizeof(sPk_tab)*/
#ifdef PC
extern int sPk_usetab[128];
#endif

#ifndef isidch
#ifdef PC
static int
isidch(c)
	char			c;
{
	return (c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z')
		|| c == '_' || c == '$';
}
static int
isbegidch(c)
	char			c;
{
	return (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c == '_' || c == '$';
}
#else
#define isidch(___c) ({register short __c=(___c); \
(__c>='0'&&__c<='9') || (__c>='A'&&__c<='Z') || (__c>='a'&&__c<='z') || __c=='_' || __c=='$';})
#define isbegidch(___c) ({register short __c=(___c); \
(__c>='A'&&__c<='Z') || (__c>='a'&&__c<='z') || __c=='_' || __c=='$';})
#endif
#endif

#ifdef PC
char sPackRet[10000];
char sPackTemp[10000];
#else
char sPackRet[1000];
char sPackTemp[1000];
#endif
int strbeg(char *S,char *s) {
	char c;
	while ((c=*s++))
		if (c!=*S++)
			return 1;
	return 0;
}
char *sPack(char *s,char *name,char *defn) {
	int nl=strlen(name),dl=strlen(defn);
	char *s0=s;
	char c,*d=sPackTemp;
	while ((c=*s)) {
		if (nl && !strbeg(s,name) && (s==s0 || (!isidch(s[-1]) && !isidch(s[nl]))))
			s+=nl,*d++='_';
		else if (dl && !strbeg(s,defn) && (s==s0 || (!isidch(s[-1]) && !isidch(s[nl]))))
			s+=dl,*d++='_';
		else *d++=*s++;
	}
	*d++=0;
	s=sPackTemp; d=sPackRet;
//return s;
	while ((c=*s)) {
#ifdef ARRAY_VER
		char pk=SPK_START; char *str,**p=sPk_tab;
		while ((str=*p++)) {
			if (!strbeg(s,str)) {
				s+=strlen(str),*d++=pk;
				goto cont;
			}
			pk++;
		}
		*d++=*s++;
#else
		char pk=SPK_START; char c,*p=sPk_table;
		while ((c=*p++)) {
			char *S=s;
			do
				if (c!=*S++) goto skip;
			while ((c=*p++));
#ifdef PC
			sPk_usetab[pk-SPK_START]++;
#endif
			p--; while (*--p); p++;
			s+=strlen(p),*d++=pk;
			goto cont;
			/*pk++;
			continue;*/
		  skip:
			while (*p++);
			pk++;
		}
		if ((char)*s<0) *d++=(char)0x80;
		*d++=*s++;
#endif
	  cont:
		(void)0;
	}
	*d++=0;
	return sPackRet;
}
#ifdef PC
char defPackRet[10000];
#else
char defPackRet[1000];
#endif
#ifdef PC
#define w(p,v) (*p++=(unsigned char)((v)>>8),*p++=(unsigned char)(v))
#else
#define w(p,_v) ({short v=_v; *p++=(unsigned char)((v)>>8); *p++=(unsigned char)(v);})
#endif
int def_is_packed=0;
char *defPack(char *s) {
	char c=0; int na=0;
	def_is_packed=0;
//return s;
//	if (!strncmp("_rom_call(",s,10)) c=(char)0xFF,s+=10,na=2;
	if (c) {
		int l=0; char *prv=s;
		char *o=defPackRet;
		*o++=c;
		while ((c=*s++)) {
			if (c=='(') l++;
			else if (c==')') { if (--l<0) s[-1]=0; w(o,atoi(prv)); }
			else if (c==',' && !l && !--na) { s[-1]=0; strcpy(o,sPack(s,"","")); while (*o++); }
		}
		return defPackRet;
	} else if (strlen(s)>4) {
		char *packed=sPack(s,"","");
		if (strcmp(packed,s)) {
			def_is_packed=1;
			strcpy(defPackRet,packed);
			return defPackRet;
		}
	}
	return s;
}
