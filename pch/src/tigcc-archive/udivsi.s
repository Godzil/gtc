.data
	.xdef __udivsi3
	.even
__udivsi3:
	move.w #0x2AA*4,%d2 /* _du32u32 */
	jra __div_entry
