.data
	.xdef calloc
	.even
calloc:
	link.w %a6,#0
	move.w 8(%a6),%d0
	mulu.w 10(%a6),%d0
	move.l %d0,-(%sp)
	move.l 0xC8,%a0
	move.l 648(%a0),%a0
	jsr (%a0)
	move.l %a0,%d0
	jbeq .L__calloc_1
	clr.w -(%sp)
	move.l %a0,-(%sp)
	move.l 0xC8,%a0
	move.l 2544(%a0),%a0
	jsr (%a0)
	jbra .L__calloc_2
.L__calloc_1:
	move.l #0,%a0
.L__calloc_2:
	unlk %a6
	rts
