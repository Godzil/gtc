/*
 * GTools C compiler
 * =================
 * source file :
 * register allocation
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"

/*
 * Register allocation (for the expression evaluation)
 * This modules handles the management of scratch registers.
 * It keeps track of the allocated registers and of the stack
 */

#ifdef MC680X0

xstatic int		next_data CGLOB, next_addr CGLOB;

#ifndef INFINITE_REGISTERS
xstatic char		dreg_in_use[MAX_DATA + 1] CGLOBL;
xstatic char		areg_in_use[MAX_ADDR + 1] CGLOBL;

xstatic struct reg_struct	reg_stack[MAX_REG_STACK + 1] CGLOBL,
							reg_alloc[MAX_REG_STACK + 1] CGLOBL;

xstatic int		reg_stack_ptr CGLOB;
xstatic int		reg_alloc_ptr CGLOB;
#endif

void g_push(int reg, enum(e_am) rmode, int number) {
#ifndef INFINITE_REGISTERS
/*
 * this routine generates code to push a register onto the stack
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+G_PUSH);
	ap->preg = reg;
	ap->mode = rmode;
	g_code(op_move, 4, ap, push_am);
	reg_stack[reg_stack_ptr].mode = rmode;
	reg_stack[reg_stack_ptr].reg = reg;
	reg_stack[reg_stack_ptr].flag = number;
	if (reg_alloc[number].flag)
		ierr(G_PUSH,1);
	reg_alloc[number].flag = 1;
	/* check on stack overflow */
	if (++reg_stack_ptr > MAX_REG_STACK)
		ierr(G_PUSH,2);
#else
	fatal("GPUSH/infinite");
#endif
}

void g_pop(int reg, enum(e_am) rmode, int number) {
#ifndef INFINITE_REGISTERS
/*
 * generate code to pop a register from the stack.
 */
	struct amode   *ap;

	/* check on stack underflow */
	if (reg_stack_ptr-- == 0)
		ierr(G_POP,1);
	/* check if the desired register really is on stack */
	if (reg_stack[reg_stack_ptr].flag != number)
		ierr(G_POP,2);
	/* check if the register which is restored is really void */
	if (rmode == am_dreg) {
		if (dreg_in_use[reg] >= 0)
			ierr(G_POP,3);
		dreg_in_use[reg] = number;
	} else {
		if (areg_in_use[reg] >= 0)
			ierr(G_POP,4);
		areg_in_use[reg] = number;
	}
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+G_PUSH);
	ap->mode = rmode;
	ap->preg = reg;
	g_code(op_move, 4, pop_am, ap);
	/* clear the push_flag */
	reg_alloc[number].flag = 0;
#endif
}

void initstack() {
/*
 * this routine should be called before each expression is evaluated to make
 * sure the stack is balanced and all of the registers are marked free.
 * This is also a good place to free all 'pseudo' registers in the
 * stack frame by setting act_scratch to zero
 */
#ifndef INFINITE_REGISTERS
	int 			i;
	next_data = 0;
	next_addr = 0;
	for (i = 0; i <= MAX_DATA; i++)
		dreg_in_use[i] = -1;
	for (i = 0; i <= MAX_ADDR; i++)
		areg_in_use[i] = -1;
	reg_stack_ptr = 0;
	reg_alloc_ptr = 0;
#else
	next_data = FIRSTREG;
	next_addr = FIRSTREG;
#endif
	act_scratch = 0;
}

//#if 0
#ifndef __HAVE_STACK_IMAGE
#define __HAVE_STACK_IMAGE
typedef struct _stackimg {
	int next_data,next_addr;
#ifndef INFINITE_REGISTERS
	int reg_alloc_ptr,reg_stack_ptr;
	char dreg_in_use[MAX_DATA+1];
	char areg_in_use[MAX_ADDR+1];
	struct reg_struct reg_stack[MAX_REG_STACK+1],reg_alloc[MAX_REG_STACK+1];
	int act_scratch;
#endif
} STACK_IMAGE;
#endif
void usestack(STACK_IMAGE *img) {	/* used by g_expr::en_compound */
	img->next_data = next_data;
	img->next_addr = next_addr;
#ifndef INFINITE_REGISTERS
	img->act_scratch = act_scratch;
	img->reg_alloc_ptr = reg_alloc_ptr;
	img->reg_stack_ptr = reg_stack_ptr;
	memcpy(img->dreg_in_use, dreg_in_use, MAX_DATA+1);
	memcpy(img->areg_in_use, areg_in_use, MAX_ADDR+1);
	memcpy(img->reg_stack, reg_stack, sizeof(struct reg_struct)*(MAX_REG_STACK+1));
	memcpy(img->reg_alloc, reg_alloc, sizeof(struct reg_struct)*(MAX_REG_STACK+1));
#endif
}
void freestack(STACK_IMAGE *img) {	/* used by g_expr::en_compound */
	next_data = img->next_data;
	next_addr = img->next_addr;
#ifndef INFINITE_REGISTERS
	act_scratch = img->act_scratch;
	reg_alloc_ptr = img->reg_alloc_ptr;
	reg_stack_ptr = img->reg_stack_ptr;
	memcpy(dreg_in_use, img->dreg_in_use, MAX_DATA+1);
	memcpy(areg_in_use, img->areg_in_use, MAX_ADDR+1);
	memcpy(reg_stack, img->reg_stack, sizeof(struct reg_struct)*(MAX_REG_STACK+1));
	memcpy(reg_alloc, img->reg_alloc, sizeof(struct reg_struct)*(MAX_REG_STACK+1));
#endif
}
//#endif

#ifdef REGPARM
#ifndef __HAVE_REGS_IMAGE
#define __HAVE_REGS_IMAGE
typedef struct _regsimg {
#ifndef INFINITE_REGISTERS
	int reg_alloc_ptr,reg_stack_ptr;
	int next_data,next_addr;
#endif
} REGS_IMAGE;
#endif
void useregs(REGS_IMAGE *img) {	/* used by g_fcall */
#ifndef INFINITE_REGISTERS
	img->reg_alloc_ptr = reg_alloc_ptr;
	img->reg_stack_ptr = reg_stack_ptr;
	img->next_data = next_data;
	img->next_addr = next_addr;
	next_data = 0;
	next_addr = 0;
#endif
}
void freeregs(REGS_IMAGE *img) {	/* used by g_fcall */
#ifndef INFINITE_REGISTERS
	int i;
	for (i = 0; i <= MAX_DATA; i++)
		dreg_in_use[i] = -1;
	for (i = 0; i <= MAX_ADDR; i++)
		areg_in_use[i] = -1;
	reg_alloc_ptr = img->reg_alloc_ptr;
	reg_stack_ptr = img->reg_stack_ptr;
	next_data = img->next_data;
	next_addr = img->next_addr;
#endif
}
#endif

#ifdef PC
void checkstack(void) {
#ifndef INFINITE_REGISTERS
/*
 * this routines checks if all allocated registers were freed
 */
		int i;
	for (i=0; i<= MAX_DATA; i++)
		if (dreg_in_use[i] != -1)
			ierr(CHECKSTACK,1);
	for (i=0; i<= MAX_ADDR; i++)
		if (areg_in_use[i] != -1)
			ierr(CHECKSTACK,2);
	if (reg_stack_ptr != 0)
		ierr(CHECKSTACK,5);
	if (reg_alloc_ptr != 0)
		ierr(CHECKSTACK,6);
#endif
	if (next_data != FIRSTREG)
		ierr(CHECKSTACK,3);
	if (next_addr != FIRSTREG)
		ierr(CHECKSTACK,4);
}
#endif

int ap_hasbeenpushed(struct amode *ap) {
#ifndef INFINITE_REGISTERS
	return reg_alloc[(int)(ap)->deep].flag;
#else
	return 0;
#endif
}

void validate(struct amode *ap) {
#ifndef INFINITE_REGISTERS
/*
 * validate will make sure that if a register within an address mode has been
 * pushed onto the stack that it is popped back at this time.
 */
	switch (ap->mode) {
	  case am_dreg:
		if (ap->preg <= MAX_DATA && reg_alloc[(int)ap->deep].flag) {
			g_pop(ap->preg, am_dreg, (int) ap->deep);
		}
		break;
	  case am_indx2:
		if (ap->sreg <= MAX_DATA && reg_alloc[(int)ap->deep].flag) {
			g_pop(ap->sreg, am_dreg, (int) ap->deep);
		}
		goto common;
	  case am_indx3:
		if (ap->sreg <= MAX_ADDR && reg_alloc[(int)ap->deep].flag) {
			g_pop(ap->sreg, am_areg, (int) ap->deep);
		}
		goto common;
	  case am_areg:
	  case am_ind:
	  case am_indx:
	  case am_ainc:
	  case am_adec:
common:
		if (ap->preg <= MAX_ADDR && reg_alloc[(int)ap->deep].flag) {
			g_pop(ap->preg, am_areg, (int) ap->deep);
		}
		break;
	}
#endif
}

struct amode *temp_data(void) {
/*
 * allocate a temporary data register and return it's addressing mode.
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+TEMP_DATA);
#ifndef INFINITE_REGISTERS
	if (dreg_in_use[next_data] >= 0)
		/*
		 * The next available register is already in use. it must be pushed
		 */
		g_push(next_data, am_dreg, dreg_in_use[next_data]);
	dreg_in_use[next_data] = reg_alloc_ptr;
#endif
	ap->mode = am_dreg;
	ap->preg = next_data;
#ifndef INFINITE_REGISTERS
	ap->deep = reg_alloc_ptr;
	reg_alloc[reg_alloc_ptr].reg = next_data;
	reg_alloc[reg_alloc_ptr].mode = am_dreg;
	reg_alloc[reg_alloc_ptr].flag = 0;
	if (next_data++ == MAX_DATA)
		next_data = 0;			/* wrap around */
	if (reg_alloc_ptr++ == MAX_REG_STACK)
		ierr(TEMP_DATA,1);
#else
	next_data++;
#endif
	return ap;
}

struct amode *temp_addr(void) {
/*
 * allocate a temporary addr register and return it's addressing mode.
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+TEMP_ADDR);
#ifndef INFINITE_REGISTERS
	if (areg_in_use[next_addr] >= 0)
		/*
		 * The next available register is already in use. it must be pushed
		 */
		g_push(next_addr, am_areg, areg_in_use[next_addr]);
	areg_in_use[next_addr] = reg_alloc_ptr;
#endif
	ap->mode = am_areg;
	ap->preg = next_addr;
#ifndef INFINITE_REGISTERS
	ap->deep = reg_alloc_ptr;
	reg_alloc[reg_alloc_ptr].reg = next_addr;
	reg_alloc[reg_alloc_ptr].mode = am_areg;
	reg_alloc[reg_alloc_ptr].flag = 0;
	if (next_addr++ == MAX_ADDR)
		next_addr = 0;			/* wrap around */
	if (reg_alloc_ptr++ == MAX_REG_STACK)
		ierr(TEMP_ADDR,1);
#else
	next_addr++;
#endif
	return ap;
}

int free_data(void) {
/*
 * returns TRUE if a data register is available at ,,no cost'' (no push).
 * Used to determine e.g. wether cmp.w #0,An or move.l An,Dm is better
 */
#ifndef INFINITE_REGISTERS
	return (dreg_in_use[next_data] < 0);
#else
	return 1;
#endif
}

void freeop(struct amode *ap) {
/*
 * release any temporary registers used in an addressing mode.
 */
	int 			number;
	if (ap == 0)
		/* This can happen freeing a NOVALUE result */
		return;
	switch (ap->mode) {
	  case am_dreg:
		if (ap->preg <= MAX_DATA) {
			if (next_data-- == 0)
				next_data = MAX_DATA;
#ifndef INFINITE_REGISTERS
			number = dreg_in_use[(int)ap->preg];
			dreg_in_use[(int)ap->preg] = -1;
#endif
			break;
		}
		return;
	  case am_indx2:
		if (ap->sreg <= MAX_DATA) {
			if (next_data-- == 0)
				next_data = MAX_DATA;
#ifndef INFINITE_REGISTERS
			number = dreg_in_use[(int)ap->sreg];
			dreg_in_use[(int)ap->sreg] = -1;
#endif
			break;
		}
		goto common;
	  case am_indx3:
		if (ap->sreg <= MAX_ADDR) {
			if (next_addr-- == 0)
				next_addr = MAX_ADDR;
#ifndef INFINITE_REGISTERS
			number = areg_in_use[(int)ap->sreg];
			areg_in_use[(int)ap->sreg] = -1;
#endif
			break;
		}
		goto common;
	  case am_areg:
	  case am_ind:
	  case am_indx:
	  case am_ainc:
	  case am_adec:
common:
		if (ap->preg <= MAX_ADDR) {
			if (next_addr-- == 0)
				next_addr = MAX_ADDR;
#ifndef INFINITE_REGISTERS
			number = areg_in_use[(int)ap->preg];
			areg_in_use[(int)ap->preg] = -1;
#endif
			break;
		}
		return;
	  default:
		return;
	}
#ifndef INFINITE_REGISTERS
	/* some consistency checks */
	if (number != ap->deep)
		ierr(FREEOP,1);
	/* we should only free the most recently allocated register */
	if (reg_alloc_ptr-- == 0)
		ierr(FREEOP,2);
	if (reg_alloc_ptr != number)
		ierr(FREEOP,3);
	/* the just freed register should not be on stack */
	if (reg_alloc[number].flag)
		ierr(FREEOP,4);
#endif
}

void temp_inv(void) {
#ifndef INFINITE_REGISTERS
/*
 * push any used temporary registers.
 * This is necessary across function calls
 * The reason for this hacking is actually that temp_inv should dump
 * the registers in the correct order,
 * the least recently allocated register first.
 * the most recently allocated register last.
 */
	int 			i;

	for (i = 0; i < reg_alloc_ptr; i++)
		if (reg_alloc[i].flag == 0) {
			g_push(reg_alloc[i].reg, reg_alloc[i].mode, i);
			/* mark the register void */
			if (reg_alloc[i].mode == am_dreg)
				dreg_in_use[reg_alloc[i].reg] = -1;
			else
				areg_in_use[reg_alloc[i].reg] = -1;
		}
#else
	g_code(_op_cleanup_for_external_call, 0, NIL_AMODE, NIL_AMODE);
#endif
}
#endif /* MC680X0 */
// vim:ts=4:sw=4
