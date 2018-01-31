.data
	.xdef __fp_entry
	.even
__fp_entry:
	link %a6,#-10
	lea (%a6,28),%a1
	bsr __fp_call
	movem.l (%a6,-10),%d0-%d1
	move.w (%a6,-2),%d2
	unlk %a6
	rts
