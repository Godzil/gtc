.data
	.xdef enter_ghost_space
	.even
enter_ghost_space:
	bsr.w .L__ghost_space_0
.L__ghost_space_0:
	move.l (%sp)+,%d0
	cmpi.l #0x40000,%d0
	bcc.s .L__ghost_space_2
	movea.l 0xC8,%a0
	cmpi.l #1000,(%a0,-4)
	bcs.s .L__ghost_space_2
	movem.l %a2-%a6/%d3-%d7,-(%sp)
	lea (%sp,-20),%sp
	move.l #0x3E000,%a3
	move.l %a0,%d0
	andi.l #0x600000,%d0
	addi.l #0x20000,%d0
	move.l %d0,(%sp,12)
	move.l %d0,(%sp,16)
	trap #0xC
	move.w #0x2700,%sr
	move.l #0xF,%d3
	pea .L__ghost_space_1(%pc)
	bset.b #2,(%sp,1)
	clr.w  -(%sp)
	move.l 0xAC,%a0
	jmp (%a0)
.L__ghost_space_1:
	lea (%sp,20),%sp
	movem.l (%sp)+,%a2-%a6/%d3-%d7
.L__ghost_space_2:
	move.l (%sp)+,%d0
	bset.l #18,%d0
	movea.l %d0,%a0
	jmp (%a0)
