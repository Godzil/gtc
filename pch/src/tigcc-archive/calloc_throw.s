.data
	.even
	.xdef calloc_throw
calloc_throw:
	lea.l .L__finished+2,%a0
	move.l (%sp)+,(%a0)
	jbsr calloc
	move.l %a0,%d0
	jbne .L__finished
	.word 0xA000+670
.L__finished:
	jmp.l 0:l
