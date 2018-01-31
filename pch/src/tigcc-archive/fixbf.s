.data
	.xdef __fixbfsi,__fixunsbfsi
	.even
__fixbfsi:
__fixunsbfsi:
	moveq.l #24,%d0
	bra __fp_entry_1
