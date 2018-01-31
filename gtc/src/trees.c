/*
 * GTools C compiler
 * =================
 * source file :
 * tree dumping
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

typedef struct dump {
} DUMP;
char *dump_node_stack[MAX_DUMP_NODE_STACK];
int dump_node_stack_depth;
int dump_attribute_phase;

#define dump_put printf
void dump_startline() {
	int i;
	for (i=0;i<dump_node_stack_depth;i++)
		dump_put("  ");
}
void dump_newnode(char *name) {
	if (dump_attribute_phase)
		dump_put(">\n");
	dump_startline();
	dump_put("<%s",name);
	dump_node_stack[dump_node_stack_depth++] = name;
	dump_attribute_phase = 1;
}
void dump_addstr(char *name,char *v) {
	assert(dump_attribute_phase);
	dump_put(" %s='%s'",name,v);
}
void dump_addint(char *name,int v) {
	char b[100];
	sprintf(b,"%d",v);
	dump_addstr(name,b);
}
void dump_addreg(char *name,int v) {
	assert(dump_attribute_phase);
	if (v>=RESULT && v<RESULT+8)
		dump_put(" %s='d%d'",name,v-RESULT);
	else if (v>=PRESULT && v<PRESULT+8)
		dump_put(" %s='a%d'",name,v-PRESULT);
	else if (v==FRAMEPTR)
		dump_put(" %s='fp'",name);
	else
		ierr(DUMP,3);
}
void dump_endnode() {
	dump_node_stack_depth--;
	if (dump_attribute_phase)
		dump_put(" />\n");
	else
		dump_startline(), dump_put("</%s>",dump_node_stack[dump_node_stack_depth]);
	dump_attribute_phase = 0;
}

void dump_genstmt(struct snode *stmt) {
	while (stmt != 0) {
		switch (stmt->type) {
			case st_compound:
				dump_genstmt(stmt->s1);
				break;
			case st_label:
				dump_newnode("st_label");
				dump_addint("id",stmt->v2.i);
				dump_endnode();
				dump_genstmt(stmt->s1);
				break;
			case st_goto:
				dump_newnode("st_goto");
				dump_addint("id",stmt->v2.i);
				dump_endnode();
				break;
			case st_break:
				dump_newnode("st_break");
				dump_endnode();
				break;
			case st_continue:
				dump_newnode("st_continue");
				dump_endnode();
				break;
			case st_expr:
				dump_newnode("st_expr");
				dump_expr(stmt->exp);
				dump_endnode();
				break;
			case st_return:
				if (!stmt->exp || ret_type->type==bt_void) {
					dump_newnode("st_return");
					dump_endnode();
					break;
				}
				switch (ret_type->type) {
					case bt_struct:
					case bt_union:
#ifdef BCDFLT
					case bt_float:
#ifdef DOUBLE
					case bt_double:
#endif
#endif
#ifdef SHORT_STRUCT_PASSING
						if (ret_type->size<=4) {
							if (stmt->exp->nodetype!=en_ref)
								ierr(DUMP,1);
							dump_newnode("st_return");
							dump_newnode("expr");
							dump_addreg("target",mk_reg(RESULT));
							dump_newnode("e_deref");
							dump_addint("esize",ret_type->size);
							dump_expr(stmt->exp->v.p[0]);
							dump_endnode();
							dump_endnode();
							dump_endnode();
							break;
						}
#endif
						assert(0);
						dump_newnode("st_return");
						dump_newnode("expr");
						dump_addstr("target","__tmp_structreturn");
						dump_expr(stmt->exp);
						dump_newnode("");
						dump_newnode("");
						break;
#ifndef BCDFLT
					case bt_float:
#ifdef DOUBLE
					case bt_double:
#endif
#endif
					case bt_char: case bt_uchar:
					case bt_short: case bt_ushort:
					case bt_long: case bt_ulong:
					case bt_pointer:
						dump_newnode("st_return");
						dump_newnode("expr");
						dump_addreg("target",ret_type->type==bt_pointer?mk_reg(PRESULT):mk_reg(RESULT));
						dump_expr(stmt->exp);
						dump_endnode();
						dump_endnode();
						break;
					default:
						ierr(DUMP,1);
				}
				break;
			case st_if:
				dump_newnode("st_if");
				dump_newnode("condition");
				dump_expr(stmt->exp);
				dump_endnode();
				dump_newnode("true");
				dump_genstmt(stmt->s1);
				dump_endnode();
				dump_newnode("false");
				dump_genstmt(stmt->v1.s);
				dump_endnode();
				dump_endnode();
				break;
			case st_asm:
				dump_newnode("st_asm");
				dump_newnode("TODO");
				//dump_asm((struct ocode *)stmt->v1.i);
				dump_endnode();
				dump_endnode();
				break;
			case st_while:
				dump_newnode("st_while");
				if (stmt->exp) {
					dump_newnode("condition");
					dump_expr(stmt->exp);
					dump_endnode();
				}
				dump_newnode("body");
				dump_genstmt(stmt->s1);
				dump_newnode("st_labelcontinue");
				dump_endnode();
				dump_endnode();
				dump_endnode();
				break;
			case st_do:
				dump_newnode(stmt->exp ? "st_do" : "st_while");
				dump_newnode("body");
				dump_genstmt(stmt->s1);
				dump_endnode();
				if (stmt->exp) {
					dump_newnode("condition");
					dump_expr(stmt->exp);
					dump_endnode();
				}
				dump_endnode();
				break;
			case st_for:
				if (stmt->exp) {
					dump_newnode("st_expr");
					dump_expr(stmt->exp);
					dump_endnode();
				}
				dump_newnode("st_while");
				dump_newnode("condition");
				dump_expr(stmt->v1.e);
				dump_endnode();
				dump_newnode("body");
				dump_genstmt(stmt->s1);
				if (stmt->v2.e) {
					dump_newnode("st_expr");
					dump_expr(stmt->v2.e);
					dump_endnode();
				}
				dump_endnode();
				dump_endnode();
				break;
			case st_switch:
			default:
				ierr(DUMP,2);
		}
		stmt = stmt->next;
	}
}

void dump_genfunc(struct snode *stmt) {
	opt1(stmt);
	dump_newnode("function");
	dump_newnode("returns");
	dump_type(ret_type);
	dump_endnode();
	dump_newnode("code");
	dump_genstmt(stmt);
	dump_genreturn(NIL_SNODE);
	dump_endnode();
	dump_endnode();
}
