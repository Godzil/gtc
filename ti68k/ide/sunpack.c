#ifdef PC
char *sUnpack(char *in,char *out,char *dic) {
	char c; char *out0=out;
	while ((c=*in++)) {
		if ((char)c>=0) *out++=c;
		else {
			if (c==(char)0x80) *out++=*in++;
			else if (c==(char)0xFF) {
				
			} else {
				char *dp=dic;
				c-=(char)0x81;
				while (c--) {
					while (*dp++);
				}
				strcpy(out,dp);
				while (*out++); out--;
			}
		}
	}
	*out++=0;
	return out0;
}
#else
char *__attribute__((stkparm)) sUnpack(char *in,char *out,char *dic);
asm(
"	xdef sUnpack\n"
"sUnpack:\n"
"/*	bra.s sUnpack*/\n"
"	move.l 4(%sp),%a0\n"
"	move.l 8(%sp),%a1\n"
"	moveq #126,%d1\n"
"	moveq #0,%d0\n"
"	move.b (%a0)+,%d0\n"
"	beq su_quit\n"
"	bmi su_special\n"
"su_copy_lp:\n"
"	move.b %d0,(%a1)+\n"
"su_next:\n"
"	move.b (%a0)+,%d0\n"
"	bgt su_copy_lp\n"
"	beq su_quit\n"
"su_special:\n"
"	subq.b #1,%d0\n"
"	bmi su_not_escape\n"
"	move.b (%a0)+,(%a1)+\n"
"	bra su_next\n"
"su_not_escape:\n"
"	addq.b #2,%d0\n"
"	bmi su_not_romcall\n"
"	\n"
"su_not_romcall:\n"
"	add.b %d1,%d0\n"
"	move.l %a0,%d2\n"
"	move.l 12(%sp),%a0\n"
"	dbf %d0,su_search_loop\n"
"	bra su_search_done\n"
"su_search_loop:\n"
"	tst.b (%a0)+\n"
"	bne su_search_loop\n"
"	dbf %d0,su_search_loop\n"
"su_search_done:\n"
"	move.b (%a0)+,(%a1)+\n"
"	bne su_search_done\n"
"	subq.w #1,%a1\n"
"	move.l %d2,%a0\n"
"	moveq #0,%d0\n"
"	bra su_next\n"
"su_quit:\n"
"	move.b	%d0,(%a1)+\n"
"	move.l 8(%sp),%a0\n"
"	rts");
#endif
