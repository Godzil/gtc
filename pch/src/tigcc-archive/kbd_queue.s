.data
	.xdef kbd_queue
	.even
kbd_queue:
	moveq.l #6,%d0
	trap #9
	rts
