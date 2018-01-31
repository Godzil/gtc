#ifndef _GENERIC_ARCHIVE
#ifdef RETURN_VALUE
#define __str(x) #x
#define __xstr(x) __str(x)
#define __var(x) x##1
#define __xvar(x) __var(x)
#if !__xvar(RETURN_VALUE)
#define RETURN_VARIABLE
#define __VN __RV+(sizeof(__xstr(RETURN_VALUE)))
extern char __RV[];
#endif
#endif
#undef RETURN_VARIABLE ///// bug in GTC!!!!!!!

#ifdef EXE_OUT
#define _NEED_COMPLEX_MAIN
#endif

#ifdef ENABLE_ERROR_RETURN
#define SPECIAL_ERROR_RETURN
#endif
asm {
#ifdef NO_VSIMPLE_MAIN
#define NO_MAIN
_main:
#define _main __main
#else
#ifdef NOSTUB
#define NO_MAIN
_nostub:
#endif
#endif
#ifdef EXE_OUT
// [params : d3=optional SYM_ENTRY* to twin ; a2=ROM_CALL table]
	move.l d3,-(a7)
	beq \no_twin_delete
#if 0
	move.l d3,-(a7)
	moveq #0,d7	// we only need the current HSym as DelTwin will make it alright
\loop
	addq.w #2,d7
	subq.l #2,d3
	move.l d3,(a7)
	jsr _ROM_CALL_23A	/* HeapPtrToHandle */
	move.w d0,(a7)
	beq \loop
	move.l (a7)+,d3
	move.w d7,d3
#else
//#ifdef FULL_EXE
//	// note that in fact the latter won't work, since SymDelTwin won't delete anything...
//	move.l d3,a0
//	move.w 12(a0),d3
//	clr.w 12(a0)	// hack so that SymDelTwin won't free the handle
//#else
	// will work as long as we don't *need* to return a locked handle (TSR...)
	// -> maybe should use full version for programs that need ghost space execution?
//#endif
#endif
	jsr _ROM_CALL_280	/* SymDelTwin */
#if 0
	move.l d3,(a7)	// save the HSym
#else
//#ifdef FULL_EXE
//	clr.l (a7)	// for HeapRealloc
//	move.w d3,-(a7)	// save the HANDLE
//	jsr _ROM_CALL_9F	/* HeapUnlock */
//	jsr _ROM_CALL_9D	/* HeapRealloc */
//#endif
#endif
\no_twin_delete
#endif
#ifdef EXECUTE_IN_GHOST_SPACE
    //--------------------------------------------------------------------------
    // get the HW parm block using the algorithm suggested by Julien Muchembled
    //--------------------------------------------------------------------------
	move.l	0xc8,d0
	and.l	#0xE00000,d0 // get the ROM base
	move.l	d0,a0
	move.l	260(a0),a1   // get pointer to the hardware param block
	add.l	#0x10000,a0
	cmp.l	a0,a1        // check if the HW parameter block is near enough
	bhs	\is_hw1or2   // if it is too far, it is HW1
	cmpi.w	#22,(a1)     // check if the parameter block contains HW ver
	bls	\is_hw1or2   // if it is too small, it is HW1

    //--------------------------------------------------------------------------
    // check for VTI (trick suggested by Julien Muchembled)
    //--------------------------------------------------------------------------
	trap	#12        // enter supervisor mode. returns old (sr) in d0.w
	move.w	#0x3000,sr // set a non-existing flag in sr (but keep s-flag)
	move.w	sr,d1      // get sr content and check for non-existing flag
	move.w	d0,sr      // restore old sr content
	btst.l	#12,d1     // this non-existing flag can only be set on the VTI
	bne	\is_hw1or2 // flag set -> VTI -> treat as HW1

    //--------------------------------------------------------------------------
    // check for HW3
    //--------------------------------------------------------------------------
	cmp.l	#3,22(a1)  // check the hardware version
	bcs	\is_hw1or2 // <3 -> proceed

    //--------------------------------------------------------------------------
    // now check for an appropriate ROM patch (HW3Patch?)
    //--------------------------------------------------------------------------
	move.l	0xc8,a0
	move.l	0xc0*4(a0),a1 // abuse EX_stoBCD
	tst.w	92(a1)        // check for HW3Patch (ROM resident)
	beq	\ghost_done   // if it is installed, we're done
	cmp.l	#0x100,0xac   // check for HW3Patch (RAM resident)
	beq	\ghost_done   // if it is installed, we're done

    //--------------------------------------------------------------------------
    // now we have a problem: we are on unpatched HW3 -> can't proceed
    //--------------------------------------------------------------------------
	pea	__hw3patch_required(pc)
	bra	\msg_exit

\is_hw1or2
    #ifndef EXE_OUT
	lea gtc_compiled,a0
	move.l a0,d0
	cmp.l #0x40000,d0
	bcc.s \ghost_done
	bset.l #18,d0
	moveq #0,d1
	move.w -2(a0),d1
	add.l d0,d1
	subq.l #1,d1
	move.l d1,-(sp)
	move.l d0,-(sp)
	move.l 0xC8,a0
	move.l 1384(a0),a0
	jsr (a0)
	addq.l #8,sp
	move.l 0xC8,a0
	cmp.l #1000,-4(a0)
	bcc.s \ghost_install
	pea \ghost_done(pc)
	bset.b #2,1(sp)
	rts
\ghost_install:
	movem.l a2-a6/d3-d7,-(sp)
	lea -20(sp),sp
	move.l #0x3E000,a3
	move.l a0,d0
	and.l #0x600000,d0
	add.l #0x20000,d0
	move.l d0,12(sp)
	move.l d0,16(sp)
	trap #12
	move.w #0x2700,sr
	moveq #0xF,d3
	pea \ghost_cont(pc)
	bset.b #2,1(sp)
	clr.w  -(sp)
	move.l 0xAC,a0
	jmp (a0)
\ghost_cont
	lea 20(sp),sp
	movem.l (sp)+,a2-a6/d3-d7
    #endif
\ghost_done
#endif
#if defined(_NEED_CALC_DETECT) || (defined(SAVE_SCREEN) && !defined(USE_KERNEL))
	move.l 0xC8,a0
#endif
#ifdef _NEED_CALC_DETECT
	moveq #1,d0
	move.l a0,d1
	and.l #0x400000,d1
	bne \calc_d0
	clr.w d0
	move.l 0x2F*4(a0),a1
	cmp.b #200,2(a1)
	blt \calc_d0
	moveq #3,d0
\calc_d0
#ifdef _NEED_CALC_VAR
	lea __calculator(pc),a1
	move.w d0,(a1)
#endif
#ifndef _SUPPORT_ALL_CALCS
#if _CALC_NUM
	subq.w #_CALC_NUM,d0
#elif !defined(_NEED_CALC_VAR)
	tst.w d0
#endif
#ifdef _ONE_CALC_ONLY
	beq \calc_ok
#else
	bne \calc_ok
#endif
	pea __wrong_calc(pc)
	bra \msg_exit
#endif
\calc_ok
#endif
#ifndef _NEED_COMPLEX_MAIN
	#ifdef NOSTUB
		bra __main
	#else
	#ifdef NO_VSIMPLE_MAIN
		bra __main
	#endif
	#endif
#else
 #ifdef NOSTUB
	#ifdef SAVE_SCREEN
		pea (a2)
		lea -3840(sp),sp
		pea 3840
		pea 0x4C00
		pea 8(sp)
		move.l 0x26A*4(a0),a2
		jsr (a2) /* memcpy */
	#endif
	#ifndef NO_EXIT_SUPPORT
		movem.l d3-d7/a2-a6,-(sp)
	#endif
 #endif
 #ifdef USE_INTERNAL_FLINE_EMULATOR
	move.l 0x2C,-(sp)
	lea \fline_handler(pc),a1
	lea 0x600000,a0
	and.w #~4,(a0)
	move.l a1,0x2C
	or.w #4,(a0)
 #endif
 #ifdef SPECIAL_ERROR_RETURN
	lea -60(sp),sp
	pea (sp)
	move.l 0xC8,a0
	move.l 0x154*4(a0),a0 /* ER_catch */
	jsr (a0)
	tst.w d0
	bne \error_returned
 #endif
 #ifdef NOSTUB
	jsr __main
 #else
	jsr __main
 #endif
 #if defined(ENABLE_ERROR_RETURN)
	move.l 0xC8,a0
	move.l 0x155*4(a0),a0 /* ER_success */
	jsr (a0)
	clr.w d0
\error_returned
	lea \error_num(pc),a0
	move.w d0,(a0)
	lea 64(sp),sp
 #endif
 #ifdef USE_INTERNAL_FLINE_EMULATOR
	lea 0x600000,a0
	and.w #~4,(a0)
	move.l (sp)+,0x2C
	or.w #4,(a0)
 #endif
 #ifdef NOSTUB
	#ifndef NO_EXIT_SUPPORT
		movem.l (sp)+,d3-d7/a2-a6
	#endif
	#ifdef SAVE_SCREEN
		pea 3840
		pea 16(sp)
		pea 0x4C00
		jsr (a2)
		lea 3864(sp),sp
		move.l (sp)+,a2
	#endif
 #endif
 #ifdef SPECIAL_ERROR_RETURN
	lea \error_num(pc),a0
	tst.w (a0)
	beq \no_error
	or.w #0xA000,(a0)
	/* for AMS 1, to unlock the prgm hd */
	move.l 0xC8,a2
	cmp.l #1000,-4(a2)
	bcc \ams1_err_ok
	pea gtc_compiled-2(pc)
	move.l 0x23A*4(a2),a0 /* HeapPtrToHandle */
	jsr (a0)
	tst.w d0
	beq \ams1_err_ok
	move.w d0,-(sp)
	move.l 0x9F*4(a2),a0 /* HeapUnlock */
	jsr (a0)
\ams1_err_ok
\error_num:
	dc.w 0
\no_error
 #endif
#ifdef EXE_OUT
\program_rts
#if 0
	move.l (a7),d0
	beq \no_twin_create
	clr.l -(a7)
	jsr _ROM_CALL_166	/* EM_twinSymFromExtMem */
	addq.l #4,a7
\no_twin_create
	addq.l #4,a7
#else
#ifdef FULL_EXE
	tst.w (a7)
	beq \no_twin_free
	jsr _ROM_CALL_97	/* HeapFree */
	addq.l #2,a7
\no_twin_free
#endif
	addq.l #4,a7
#endif
#ifdef BSS_SUPPORT
	pea __bss_start
	jsr _ROM_CALL_A3	/* tios::HeapFreePtr */
	addq.l #4,a7
#endif
	movem.l (a7)+,d3-d7/a2-a6
	move.l 0xC8,a0
	move.l 0x97*4(a0),a0	/* tios::HeapFree */
	jmp (a0)	// note : we can't use a _ROM_CALL here, as we need a jmp
 #ifdef RETURN_VALUE
 #error Not compatible yet.
 #endif
#else
 #ifndef RETURN_VALUE
	rts
 #else
  #ifdef NOSTUB
   #ifndef RETURN_VARIABLE
	move.l (sp)+,a0
	cmpi.w #0x21EE,(a0)
	bne.s \A2
	addq.l #2,a0
\A2:
	jmp 4(a0)
   #else
	move.l 0xC8,a0
	move.l 1060(a0),a1
	move.l (a1),-(sp)
	move.l #0x40000000,-(sp)
	pea __VN(pc)
	move.l 0x86*4(a0),a0
	jsr (a0) /* VarStore */
	lea 12(sp),sp
	rts
   #endif
  #else
   #ifndef RETURN_VARIABLE
	move.l _ROM_CALL_109,_RAM_CALL_00F
	rts
   #else
	move.l _ROM_CALL_109,-(sp)
	move.l #0x40000000,-(sp)
	pea __VN(pc)
	jsr _ROM_CALL_86
	lea 12(sp),sp
	rts
   #endif
  #endif
 #endif
/* #ifdef ENABLE_ERROR_RETURN
\error_num:
// move.w #0,#8
 dc.w 0
 #endif*/
#endif
#endif
#ifdef USE_INTERNAL_FLINE_EMULATOR
\fline_handler:
	move.w (sp)+,d0
	move.l (sp)+,a0
	move.w d0,sr
	move.w (a0)+,d0
	and.w #0x7FF,d0
	lsl.w #2,d0
	move.l 0xC8,a1
	pea (a0)
	move.l 0(a1,d0.w),a0
	jmp (a0)
#endif
#if (defined(_NEED_CALC_DETECT)&&!defined(_SUPPORT_ALL_CALCS)) || defined(EXECUTE_IN_GHOST_SPACE)
\msg_exit:
	move.l 0xE6*4(a0),a0 /* ST_helpMsg */
	jsr (a0)
	addq.l #4,a7
#ifdef EXE_OUT
	bra \program_rts
#else
#ifdef NOSTUB
	rts
#else
	jmp _ROM_CALL_51 /* ngetchx */
#endif
#endif
#endif
#if defined(_NEED_CALC_DETECT) && defined(_NEED_CALC_VAR)
	even
__calculator:
	dc.w -1
#endif
#ifndef NO_EXIT_SUPPORT
__save__sp__:
	dc.l 0
#endif
	even
};

#if (defined(_NEED_CALC_DETECT)&&!defined(_SUPPORT_ALL_CALCS))
char __wrong_calc[]="Wrong calculator model";
#endif
#ifdef EXECUTE_IN_GHOST_SPACE
char __hw3patch_required[]="HW3Patch required";
#endif
#ifdef RETURN_VARIABLE
char __RV[]="\0" __xstr(RETURN_VALUE);
#endif
#endif
