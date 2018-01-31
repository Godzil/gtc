.data
	.xdef realloc
	.even
realloc:
	link.w %a6,#0
	movm.l %d3/%a2-%a3,-(%sp)
	move.l 0xC8,%a2
	move.l 12(%a6),%a3 /* new size */
	tst.l 8(%a6) /* address==0? */
	jbeq .L__realloc_0
	move.l 8(%a6),%a0 /* address */
	move.w -2(%a0),%d3 /* handle */
	move.w %d3,-(%sp)
	move.l 636(%a2),%a0 /* HeapUnlock */
	jsr (%a0)
	pea 2(%a3) /* reallocate a block of size+2 bytes */
	move.w %d3,-(%sp)
	move.l 628(%a2),%a0 /* HeapRealloc */
	jsr (%a0)
	tst.w %d0
	jbne .L__realloc_1
	move.l 604(%a2),%a0 /* HeapFree */
	jsr (%a0)
	sub.l %a0,%a0
	jbra .L__realloc_2
.L__realloc_0:
	pea (%a3)
	move.l 648(%a2),%a0 /* HeapAllocPtr */
	jsr (%a0)
	jbra .L__realloc_2
.L__realloc_1:
	move.l 612(%a2),%a0 /* HLock */
	jsr (%a0)
	addq.l #2,%a0 /* skip the handle */
.L__realloc_2:
	movm.l -12(%a6),%d3/%a2-%a3
	unlk %a6
	rts
