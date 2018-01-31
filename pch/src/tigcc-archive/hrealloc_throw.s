| Warning: This routine has the attribute __ATTR_TIOS__!
| It is a simple wrapper for a TIOS function.

.data
	.even
	.xdef HeapReallocThrow
HeapReallocThrow:
	lea.l .L__finished+2,%a0
	move.l (%sp)+,(%a0)
	move.l 0xC8,%a0
	move.l (%a0,0x9D*4),%a0 /* HeapRealloc */
	jsr (%a0)
	tst.w %d0
	jbne .L__finished
	.word 0xA000+670
.L__finished:
	jmp.l 0:l
