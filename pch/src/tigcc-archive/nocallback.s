| Warning: This routine has the attribute __ATTR_TIOS_CALLBACK__!

.data
	.xdef NoCallBack
	.even
NoCallBack:
	moveq.l #1,%d0
	rts
