#ifdef VCG
#ifdef PC
#define MAXINT 0x7FFFFFFF
#endif
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
STACK_IMAGE vcg_img[VCG_MAX+1];
int vcg_nxl[VCG_MAX+1];
int vcg_aborted[VCG_MAX+1];
int vcg_init() {
	if (--vcg_lvl<0) {
		vcg_lvl++;
		return 0;
	}
//	tmp_use();
	usestack(&vcg_img[vcg_lvl]);
	vcg_peep_head[vcg_lvl]=0;
	vcg_aborted[vcg_lvl]=0;
	vcg_nxl[vcg_lvl]=nextlabel;
	g_code(op_label,0,0,0);
	return 1;
//	vcg_on++;
//	vcg_cost[vcg_lvl]=0;
}
int en_dir_cost(struct enode *ep) {
	switch (ep->nodetype) {
		case en_icon:
			return (ep->v.i>=-32768 && ep->v.i<32767)?1:2;
		case en_labcon:
		case en_nacon:
			return 1;
		case en_add:
		case en_sub:
			return max(en_dir_cost(ep->v.p[0]),en_dir_cost(ep->v.p[1]));
	}
}
int cost_tab[] = {
	0,0,0,0,0,1,1,1,-MAXINT-1,2,1,1,1,1,2,0,0
};
int vcg_cost() {
	int cost=0;
	if (!vcg_aborted[vcg_lvl]) {
		struct ocode *ip;
		opt3();
		ip = peep_head;
		while (ip != 0) {
	#define am_cost(x) (x?(x->mode==am_direct?en_dir_cost(x->offset):cost_tab[x->mode]):0)
			cost++;
			switch (ip->opcode) {
			case op_label:
			case op_even:
				cost--;
				break;
			case op_moveq:
			case op_addq: case op_subq:
			case op_lsl: case op_lsr: case op_asl: case op_asr:
			case op_rol: case op_ror: case op_roxl: case op_roxr:
			case op_trap:
				cost+=am_cost(ip->oper2);
				break;
			case op_bxx:
				/* what should we do here? */
				break;
			case op_dbxx:
				cost++;
				break;
			default:
				cost+=am_cost(ip->oper1)+am_cost(ip->oper2);
				break;
			}
			if (cost<0) cost+=(-MAXINT-1)+((ip->length+1)>>1);
			ip = ip->fwd;
		}
	//	vcg_on--;
	//	tmp_free();
	} else cost=12345;
	return cost;
}
int vcg_done() {
	freestack(&vcg_img[vcg_lvl]);
	nextlabel=vcg_nxl[vcg_lvl];
	int cost=vcg_cost();
	vcg_lvl++;
	return cost;
}
#endif
// vim:ts=4:sw=4
