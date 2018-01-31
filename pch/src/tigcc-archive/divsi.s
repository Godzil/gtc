.data
	.xdef __divsi3
	.even
__divsi3:
	move.w #0x2A8*4,%d2 /* _ds32s32 */
	jra __div_entry
