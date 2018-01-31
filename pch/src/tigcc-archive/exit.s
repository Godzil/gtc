|exit function copyright (C) 2002, Kevin Kofler
|requires new exit support (__save__sp__)
|Thanks to Patrick Pélissier for the code. I have just changed it from a patch
|to an archive function, so it won't be there when not needed.

.data
	.xdef __exit
__exit:
	movea.l __save__sp__:l,%a7 |(NOT PC-relative because of programs >32 KB)
	rts
