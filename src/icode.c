/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

/* constant AST*/
ast *ast_cnst_zero;
ast *ast_cnst_one;
symbol *cnst_one;
symbol *cnst_zero;
//enum {psw_equ, psw_unequ};
#define new_label(lab)	emit_ic(IC_LABEL, lab, NULL)
#define is_ast_asgn(t)	(t >= AST_ASGN && t <= AST_BIOR_ASGN)
static void stmt2ic(ast *t1);
static symbol *expr2ic(ast *t1);
static symbol *unary2ic(ast *t1);
static symbol *asgn2ic(ast *t1);
static void block2ic(ast *t);

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
int gen_post(int tc, int fc) {
	assert(is_scalar_cat(tc) && is_scalar_cat(fc));
	switch (tc) {
	case CAT_CHAR:
	case CAT_UCHAR:
		switch (fc) {
		case CAT_CHAR:
		case CAT_UCHAR:
			return i8;
		case CAT_INT:
		case CAT_UINT:
		case CAT_POINTER:
			/* i32 -> i8*/
			return i8_i32;
		case CAT_FLOAT:
			/* f32 -> i8*/
			return i8_f32;
		case CAT_DOUBLE:
			return i8_f64;
		}
	case CAT_INT:
	case CAT_UINT:
	case CAT_POINTER:
		switch (fc) {
		case CAT_CHAR:
		case CAT_UCHAR:
			/* i8 -> i32*/
			return i32_i8;
		case CAT_INT:
		case CAT_UINT:
		case CAT_POINTER:
			return i32;
			//return i32_p32;
		case CAT_FLOAT:
			/* f32 -> i32*/
			return i32_f32;
		case CAT_DOUBLE:
			/* f64 -> i32*/
			return i32_f64;
		}
	case CAT_FLOAT:
		switch (fc) {
		case CAT_CHAR:
		case CAT_UCHAR:
			/* i8 -> f32*/
			return f32_i8;
		case CAT_INT:
		case CAT_UINT:
		case CAT_POINTER:
			return f32_i32;
		case CAT_FLOAT:
			return f32;
		case CAT_DOUBLE:
			return f32_f64;
		}
	case CAT_DOUBLE:
		switch (fc) {
		case CAT_CHAR:
		case CAT_UCHAR:
			return f64_i8;
		case CAT_INT:
		case CAT_UINT:
		case CAT_POINTER:
			return f64_i32;
		case CAT_FLOAT:
			return f64_f32;
		case CAT_DOUBLE:
			return f64;
		}
	}

	assert(0);
}

/*===--------------------------------------------------------------------------
 after this itmp is means virtual register
---------------------------------------------------------------------------===*/
static symbol *gen_vit_reg(type *ty) {
	symbol *p;
	assert(ty);
	assert(is_scalar_cat(ty->cat));
	/* do we need PERM ?*/
	NEW0(p, FUNC);
	//p->name = gen_tmp_name("i");
	p->is_itmp = 1;
	p->is_split = 1;		//has no register
	p->ty = ty;
	if (ty->cat == CAT_FLOAT || ty->cat == CAT_DOUBLE)
		p->is_need_st = 1;

	return p;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void add2ic_list(icode *ic) {
	_F.tail_ic->next = ic;
	ic->prev = _F.tail_ic;
	_F.tail_ic = ic;

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static icode *emit_ic(int node, symbol *l, symbol *r) {
	icode *ic;

	if (node != IC_MOV)
		assert(l->ty->cat == r->ty->cat);
	NEW0(ic, IC);
	ic->node = node;
	ic->left = l;
	ic->right = r;
	add2ic_list(ic);
	/* updata ues times*/
	if (l)
		l->use_times++;
	if (r)
		r->use_times++;

	return ic;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static void verify_ops(symbol **s1, symbol *s2) {
	symbol *r;
	int post, cat;
	
	cat = (*s1)->ty->cat;
	post = gen_post(cat, cat);
	// two memory operand or left operand is immediate value
	if ((*s1)->is_itmp == 0 && s2->is_itmp == 0 || (*s1)->is_immediate) {
		r = gen_vit_reg((*s1)->ty);
		emit_ic(IC_MOV + post, r, *s1);
		*s1 = r;
	}

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void emit_inc(int ic, symbol *s) {
	int cat, post;
	symbol *val;
	value v;

	assert(s->ty);
	cat = s->ty->cat;
	post = gen_post(cat, cat);
	//s1 = cast(l, ty);
	if (cat == CAT_POINTER) {
		v.i = s->ty->bty->size;
		val = constant(v, int_type);
		if (ic == IC_DEC)
			ic = IC_SUB;
		else if (ic == IC_INC)
			ic = IC_ADD;
		else
			assert(0);
		emit_ic(ic + i32, s, val);
	} else
		emit_ic(ic + post, s, NULL);

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static symbol *emit_member(symbol *base, symbol *s) {
	int pos, len, size;
	type *ty = s->ty;
	symbol *ls_len, *rs_len, *s1, *os;
	value v;

	size = ty->size;
	s1 = gen_vit_reg(ty);
	v.i = s->u.s.offset;
	os = constant(v, int_type);
	emit_ic(IC_ADD + i32, base, os);	//base + offset
	emit_ic(IC_MOV + gen_post(ty->cat, ty->cat), s1, base);
	if (s->is_bitfield) {
		pos = s->u.s.bits_pos;
		len = s->u.s.bits_len;
		v.i = pos;
		rs_len = constant(v, int_type);
		emit_ic(IC_LS + i32, s1, rs_len);
		v.i = size - pos - len;
		ls_len = constant(v, int_type);
		emit_ic(IC_RS + i32, s1, ls_len);
	}

	return s1;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static symbol *cast(symbol *from, type *to) {
	symbol *dst;//, *s1, *s2;
	int c1, c2, post;

	c1 = to->cat;
	c2 = from->ty->cat;
	if (c1 == c2)
		return from;
	assert(is_scalar_cat(c1));
	assert(is_scalar_cat(c2));
	/* the hard part*/
	dst = gen_vit_reg(to);
	post = gen_post(c1, c2);
	emit_ic(IC_CVT + post, dst, from);
#if 0	
	/* ths symbol from may be is a itmp, but dst symbol must be a itmp*/
	switch (post) {
	case i8_i32:
		emit_ic(IC_MOV + post, dst, from);					//movsbl/movb
		break;
	case i8_f32:
		s1 = gen_vit_reg(float_type);
		emit_ic(IC_MOV + f80_f32, s1, from);				// fld src
		/** 
			Attention, the type of dst is char,
			fst instruction must store to a m32int
		*/
		emit_ic(IC_MOV + i32_f80, _F.spill_i32, s1);			// fist	
		emit_ic(IC_MOV + i8_i32, dst, _F.spill_i32);
		break;
	case i8_f64:
		s1 = gen_vit_reg(double_type);
		emit_ic(IC_MOV + f80_f64, s1, from);				// fld src
		//	dst is a i8 register
		emit_ic(IC_MOV + i32_f80, _F.spill_i32, s1);			// fist	
		emit_ic(IC_MOV + i8_i32, dst, _F.spill_i32);
		break;
	case i32_i8:
		emit_ic(IC_MOV + post, dst, from);				//movsbl/movb
		break;
	case i32_f32:
		s1 = gen_vit_reg(float_type);
		emit_ic(IC_MOV + f80_f32, s1, from);			// fld src
		emit_ic(IC_MOV + f32_f80, _F.spill_i32, s1);		// fist	
		emit_ic(IC_MOV + i32, dst, _F.spill_i32);
		break;
	case i32_f64:
		s1 = gen_vit_reg(double_type);
		emit_ic(IC_MOV + f80_f64, s1, from);			// fld src
		emit_ic(IC_MOV + i32_f80, _F.spill_i32, s1);		// fist	
		emit_ic(IC_MOV + i32, dst, _F.spill_i32);
		break;
	case f32_i8:	// i8 --> f32
		s1 = gen_vit_reg(int_type);
		emit_ic(IC_MOV + i32_i8, s1, from);
		/* when the fpu stack has be used everytime, zcc simply store the value*/
		// type of des is float
		emit_ic(IC_MOV + i32, _F.spill_i32, s1);
		emit_ic(IC_MOV + f80_i32, dst, _F.spill_i32);
		break;
	case f32_i32:
		s1 = gen_vit_reg(int_type);
		emit_ic(IC_MOV + i32, s1, from);
		emit_ic(IC_MOV + i32, _F.spill_i32, s1);
		emit_ic(IC_MOV + f80_i32, dst, _F.spill_i32);
		break;
	case f32_f64:
		emit_ic(IC_MOV + f80_f64, dst, from);
		break;
	case f64_i8:
		s1 = gen_vit_reg(int_type);
		emit_ic(IC_MOV + i32_i8, s1, from);
		emit_ic(IC_MOV + i32, _F.spill_i32, s1);
		emit_ic(IC_MOV + f80_i32, dst, _F.spill_i32);
		break;
	case f64_i32:
		s1 = gen_vit_reg(int_type);
		emit_ic(IC_MOV + i32, s1, from);
		emit_ic(IC_MOV + i32, _F.spill_i32, s1);
		emit_ic(IC_MOV + f80_i32, dst, _F.spill_i32);
		break;
	case f64_f32:
		/* if from is itmp, it must be st0, otherwise fld from operand*/
		emit_ic(IC_MOV + f80_f32, dst, from);
		break;
	default:
		assert(0);
	}
#endif

	return dst;
}

/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static symbol *primary2ic(ast *t1) {
	switch (t1->node) {
	case AST_ID:
		return t1->u.id;
	case AST_CNST:
		assert(t1->u.cnst->is_immediate);	//instruction immediate data
		return t1->u.cnst;
	case AST_PACK_EXPR:
		return expr2ic(t1->left);
	default:
		assert(0);
	}
}

/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static symbol *cast2ic(ast *t1) {
	symbol *s1;
	if (t1->node == AST_CAST) {
		s1 = cast2ic(t1->right);
		return cast(s1, t1->ty);
	} else
		return unary2ic(t1);
}

/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static symbol *post2ic(ast *t1) {
	symbol *s1, *s2, *val, *base;
	dlist *args, *d2;
	ast *t2;
	int node = t1->node, cat, post;
	type *ty = t1->ty;
	value v;
	icode *ic;

	assert(ty && "type of post AST empty");
	cat = ty->cat;		// t1->ty->cat
	switch (node) {
	case AST_POST_INC:
	case AST_POST_DEC:
		/*-----------------------------------------
			char c = 1;	//char
			c++;		//int
		ty is scalar(int/float/double) type
		------------------------------------------*/
		s1 = post2ic(t1->left);
		val = gen_vit_reg(ty);
		emit_ic(IC_MOV + gen_post(cat, cat), val, cast(s1, ty));
		/// source's post value
		post = gen_post(s1->ty->cat, s1->ty->cat);
		if (node == AST_POST_INC)
			emit_inc(IC_INC, s1);
		else
			emit_inc(IC_DEC, s1);
		return val;
	case AST_ARRAY:
		s1 = post2ic(t1->left);			//base
		s2 = expr2ic(t1->right);		//index
		assert(is_int_cat(s2->ty->cat));
		assert(s1->ty->cat == CAT_POINTER);
		// the type infer of post-expression is work correct ?
		assert(s1->ty->cat == ty->cat);
		/* size of array's element*/
		assert(ty->size);
		v.i = ty->size;
		s2 = cast(s2, int_type);
		val = constant(v, int_type);
		verify_ops(&s2, val);		// make sure s2 is a not a immediate value
		emit_ic(IC_MUL + i32, s2, val);	//offset
		/* address = base + offset, can be optimzation using complex address mode*/
		base = gen_vit_reg(s1->ty);
		emit_ic(IC_ADDR + i32, base, s1);
		emit_ic(IC_ADD + i32, s2, base);	//base + offset
		val = gen_vit_reg(ty);
		val->is_addr = 1;
		emit_ic(IC_MOV + i32, val, s2);
		return val;			// lvalue, is a address
	case AST_CALL:
		s1 = post2ic(t1->left);
		args = (dlist *)t1->right->left;
		d2 = args = args->prev;	//pass argument from right to left
		if (d2)
			do {
				t2 = (ast *)d2->it;
				s2 = asgn2ic(t2);
				//emit_pass_arg(t2->u.arg_off, s2);
				// TODO, pushl arguments ?????
				post = gen_post(t2->ty->cat, t2->ty->cat);
				ic = emit_ic(IC_MOV_ARG + post, s2, NULL);
				ic->u.arg_off = t2->u.arg_off;
				d2 = d2->prev;
			} while (d2 != args);
		if (ty->cat != CAT_VOID)
			val = gen_vit_reg(ty);
		else {		// no return value
			assert(ty == void_type);
			val = NULL;
		}
		emit_ic(IC_CALL, val, s1);
		return val;
	case AST_MEMBER:
		s1 = post2ic(t1->left);
		s2 = primary2ic(t1->right);
		val = emit_member(s1, s2);
		return val;
	case AST_ARROW:
		s1 = post2ic(t1->left);
		s2 = primary2ic(t1->right);
		val = gen_vit_reg(s1->ty);
		emit_ic(IC_MOV + i32, val, s1);		// get the address value if in lvalue
		val = emit_member(val, s2);
		return val;	//lvalue, is a address
	default:
		return primary2ic(t1);
	}
	assert(0);
}

/*===--------------------------------------------------------------------------
change the AST of unary-expression to icode
--------------------------------------------------------------------------===*/
static symbol *unary2ic(ast *t1) {
	symbol *s, *val, *s8;
	int node = t1->node, fc, tc;
	type *ty = t1->ty;
	
	//assert(t1->right);
	if (node == AST_PRE_INC || node == AST_PRE_DEC)
		s = unary2ic(t1->right);
	else if (node == AST_CAST)
		s = cast2ic(t1->right);
	assert(ty);
	tc = ty->cat;
	switch (node) {
	case AST_PRE_INC:
	case AST_PRE_DEC:
		// ++(*p)
		//assert(s->is_itmp == 0);
		assert(tc == CAT_INT || tc == CAT_UINT);
		if (node == AST_PRE_INC)
			emit_inc(IC_INC, s);
		else
			emit_inc(IC_DEC, s);
		val = gen_vit_reg(ty);
		/* ++id can not generate lvalue, so we must has this icode*/
		emit_ic(IC_MOV + gen_post(tc, tc), val, cast(s, ty));
		return val;
	case AST_ADDR:
		//assert(s->is_itmp == 0);
		val = gen_vit_reg(ty);
		assert(ty->cat == CAT_POINTER);
		// if pointer, gen_post() return i32
		emit_ic(IC_ADDR + i32, val, s);
		return val;
	case AST_IND:
		fc = s->ty->cat;
		/* Attention: must generate a lvalue*/
		assert(fc == CAT_POINTER);
		val = gen_vit_reg(ty);
		val->is_addr = 1;
		emit_ic(IC_MOV + i32, val, s);
		return val;	//address
	case AST_PST:
		/* Attention this cast*/
		return cast(s, ty);
	case AST_NEG:	// -
		val = gen_vit_reg(ty);
		emit_ic(IC_MOV + gen_post(tc, tc), val, cast(s, ty));
		emit_ic(IC_NEG + gen_post(tc, tc), val, NULL);
		return val;
	case AST_COM:	// ~
		val = gen_vit_reg(ty);
		emit_ic(IC_MOV + gen_post(tc, tc), val, cast(s, ty));
		emit_ic(IC_NOT + gen_post(tc, tc), val, NULL);
		return val;
	case AST_NOT:	// !
		assert(ty->cat == CAT_INT);
		emit_ic(IC_CMP_0 + i32, cast(s, int_type), NULL);
		s8 = gen_vit_reg(char_type);
		//emit_ic(IC_MOV + i32_i8, s8, );
		emit_ic(IC_SETE, s8, NULL);
		return cast(s8, int_type);
	default:
		return post2ic(t1);
	}

	assert(0);
}

/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static void emit_branch(int node, symbol *dst_addr) {
	icode *ic1;
	symbol *s1 = dst_addr;

	ic1 = emit_ic(node, s1, NULL);
	add2dlist_head(&s1->u.l.refs_ic, ic1, FUNC);
	
	return;
}

/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static int eval_icode_node(int node) {
	switch (node) {
	case AST_BIOR:
		return IC_BIOR;
	case AST_BXOR:
		return IC_BXOR;
	case AST_BAND:
		return IC_BAND;
	case AST_LS:
		return IC_LS;
	case AST_RS:
		return IC_RS;
	case AST_MUL:
		return IC_MUL;
	case AST_DIV:
		return IC_DIV;
	case AST_MOD:
		return IC_MOD;
	case AST_ADD:
		return IC_ADD;
	case AST_SUB:
		return IC_SUB;
	default:
		assert(0);
	}
}

/*===-------------------------------------------------------------------------
 * / % + - >> <<
-------------------------------------------------------------------------===*/
static symbol *arith_expr2ic(ast *root) {
	ast *t = root, *t1, *t2;
	symbol *s1, *s2, *val;
	type *dt;
	int ic;
	int set[] = {AST_MUL, AST_MOD, AST_DIV, AST_ADD, AST_SUB, AST_LS, AST_RS, 0};
	int mul_set[] = {AST_MUL, AST_MOD, AST_DIV, 0};

	if (!is_in_set(t->node, set))
		return cast2ic(t);
	// s1 = expr1
	t1 = t->left;
	if (is_in_set(t->node, mul_set))
		s1 = cast2ic(t1);
	else
		s1 = arith_expr2ic(t1);
	// more than one operator of equal precedence
	ic = eval_icode_node(t->node);
	dt = t->ty;
	t = t->right;
	while (is_in_set(t->node, set)) {
		t2 = t->left;
		if (is_in_set(t->node, mul_set))
			s2 = cast2ic(t2);
		else
			s2 = arith_expr2ic(t2);
		s1 = cast(s1, dt);
		val = cast(s2, dt);
		verify_ops(&s1, val);
		emit_ic(ic + gen_post(dt->cat, dt->cat), s1, val);
		///
		ic = eval_icode_node(t->node);
		dt = t->ty;
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	if (is_in_set(t->node, mul_set))
		s2 = cast2ic(t);
	else
		s2 = arith_expr2ic(t);
	s1 = cast(s1, dt);
	s2 = cast(s2, dt);
	verify_ops(&s1, s2);
	emit_ic(ic + gen_post(dt->cat, dt->cat), s1, s2);

	return s1;
}

/*===-------------------------------------------------------------------------
expr1 * expr2 * expr3
-------------------------------------------------------------------------===*/
static symbol *rel_expr2ic(ast *root) {
	ast *t = root, *t1, *t2;
	symbol *s1, *s2, *val, *lab;
	type *dt, *ty = t->ty;
	int ic;

	assert(t->node != AST_G && t->node != AST_GE);
	if (t->node != AST_L && t->node != AST_LE)
		return arith_expr2ic(t);
	assert(ty == int_type);
	// s1 = expr1
	t1 = t->left;
	s1 = arith_expr2ic(t1);
	// more than one operator of equal precedence
	if (t->node == AST_L)
		ic = IC_JL;
	else
		ic = IC_JLE;
	t = t->right;
	while (t->node == AST_L || t->node == AST_LE) {
		t2 = t->left;
		s2 = arith_expr2ic(t2);
		dt = get_arith_type(s1->ty, s2->ty);
		emit_ic(IC_CMP + gen_post(dt->cat, dt->cat), cast(s1, dt), cast(s2, dt));
		lab = gen_label(NULL);
		emit_branch(ic, lab);
		val = gen_vit_reg(ty);
		emit_ic(IC_MOV + i32, val, cnst_zero);
		new_label(lab);
		emit_ic(IC_MOV + i32, val, cnst_one);
		///
		s1 = val;
		if (t->node == AST_L)
			ic = IC_JL;
		else
			ic = IC_JLE;
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = arith_expr2ic(t);
	dt = get_arith_type(s1->ty, s2->ty);
	emit_ic(IC_CMP + gen_post(dt->cat, dt->cat), cast(s1, dt), cast(s2, dt));
	lab = gen_label(NULL);
	emit_branch(ic, lab);
	val = gen_vit_reg(ty);
	emit_ic(IC_MOV + i32, val, cnst_zero);
	new_label(lab);
	emit_ic(IC_MOV + i32, val, cnst_one);

	return val;
}

/*===-------------------------------------------------------------------------
expr1 * expr2 * expr3
-------------------------------------------------------------------------===*/
static symbol *equ_expr2ic(ast *root) {
	ast *t = root, *t1, *t2;
	symbol *s1, *s2, *val, *lab;
	type *dt, *ty = t->ty;
	int pre;

	if (t->node != AST_EQU && t->node != AST_UE)
		return rel_expr2ic(t);
	assert(ty == int_type);
	// s1 = expr1
	t1 = t->left;
	s1 = rel_expr2ic(t1);
	// more than one operator of equal precedence
	//ic = eval_icode_node(t->node);
	pre = t->node;
	t = t->right;
	while (t->node == AST_EQU || t->node == AST_UE) {
		t2 = t->left;
		s2 = rel_expr2ic(t2);
		dt = get_arith_type(s1->ty, s2->ty);
		emit_ic(IC_CMP + gen_post(dt->cat, dt->cat), cast(s1, dt), cast(s2, dt));
		lab = gen_label(NULL);
		emit_branch(IC_JE, lab);
		val = gen_vit_reg(ty);
		if (pre == AST_EQU)
			emit_ic(IC_MOV + i32, val, cnst_zero);
		else
			emit_ic(IC_MOV + i32, val, cnst_one);
		new_label(lab);
		if (pre == AST_EQU)
			emit_ic(IC_MOV + i32, val, cnst_one);
		else
			emit_ic(IC_MOV + i32, val, cnst_zero);
		///
		s1 = val;
		pre = t->node;
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = rel_expr2ic(t);
	dt = get_arith_type(s1->ty, s2->ty);
	emit_ic(IC_CMP + gen_post(dt->cat, dt->cat), cast(s1, dt), cast(s2, dt));
	lab = gen_label(NULL);
	emit_branch(IC_JE, lab);
	val = gen_vit_reg(ty);
	if (pre == AST_EQU)
		emit_ic(IC_MOV + i32, val, cnst_zero);
	else
		emit_ic(IC_MOV + i32, val, cnst_one);
	new_label(lab);
	if (pre == AST_EQU)
		emit_ic(IC_MOV + i32, val, cnst_one);
	else
		emit_ic(IC_MOV + i32, val, cnst_zero);

	return val;
}

/*===-------------------------------------------------------------------------
expr1 * expr2 * expr3
-------------------------------------------------------------------------===*/
static symbol *band_expr2ic(ast *root) {
	ast *t = root, *t1, *t2;
	symbol *s1, *s2;
	type *dt = int_type;

	if (t->node != AST_BAND)
		return equ_expr2ic(t);
	// s1 = expr1
	t1 = t->left;
	s1 = equ_expr2ic(t1);
	// more than one operator of equal precedence
	t = t->right;
	while (t->node == AST_BAND) {
		t2 = t->left;
		s2 = equ_expr2ic(t2);
		s1 = cast(s1, dt);
		cast(s2, dt);
		verify_ops(&s1, s2);
		emit_ic(IC_BAND + i32, s1, s2);
		///
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = equ_expr2ic(t);
	s1 = cast(s1, dt);
	s2 = cast(s2, dt);
	verify_ops(&s1, s2);
	emit_ic(IC_BAND + i32, s1, s2);

	return s1;
}

/*===-------------------------------------------------------------------------
expr1 * expr2 * expr3
-------------------------------------------------------------------------===*/
static symbol *bxor_expr2ic(ast *root) {
	ast *t = root, *t1, *t2;
	symbol *s1, *s2;
	type *dt = int_type;

	if (t->node != AST_BXOR)
		return band_expr2ic(t);
	// s1 = expr1
	t1 = t->left;
	s1 = band_expr2ic(t1);
	// more than one operator of equal precedence
	t = t->right;
	while (t->node == AST_BXOR) {
		t2 = t->left;
		s2 = band_expr2ic(t2);
		s1 = cast(s1, dt);
		s2 = cast(s2, dt);
		verify_ops(&s1, s2);
		emit_ic(IC_BXOR + i32, s1, s2);
		///
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = band_expr2ic(t);
	s1 = cast(s1, dt);
	s2 = cast(s2, dt);
	verify_ops(&s1, s2);
	emit_ic(IC_BXOR + i32, s1, s2);

	return s1;
}

/*===-------------------------------------------------------------------------
expr1 * expr2 * expr3
-------------------------------------------------------------------------===*/
static symbol *bior_expr2ic(ast *root) {
	ast *t = root, *t1, *t2;
	symbol *s1, *s2;
	type *dt = int_type;

	if (t->node != AST_BIOR)
		return bxor_expr2ic(t);
	// s1 = expr1
	t1 = t->left;
	s1 = bxor_expr2ic(t1);
	// more than one operator of equal precedence
	t = t->right;
	while (t->node == AST_BIOR) {
		t2 = t->left;
		s2 = bxor_expr2ic(t2);
		s1 = cast(s1, dt);
		s2 = cast(s2, dt);
		verify_ops(&s1, s2);
		emit_ic(IC_BIOR + i32, s1, s2);
		///
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = bxor_expr2ic(t);
	s1 = cast(s1, dt);
	s2 = cast(s2, dt);
	verify_ops(&s1, s2);
	emit_ic(IC_BIOR + i32, s1, s2);

	return s1;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static symbol *and_expr2ic(ast *t) {
	ast *t1, *t2;
	symbol *s1, *s2, *val, *lab;
	type *dt = int_type;

	if (t->node != AST_AND)
		/* less precedent expression*/
		return bior_expr2ic(t);
	// branch label
	lab = gen_label(NULL);
	// s1 = expr1
	t1 = t->left;
	s1 = bior_expr2ic(t1);
	emit_ic(IC_CMP_0 + i32, s1, NULL);
	emit_branch(IC_JE, lab);
	// more than one operator of equal precedence
	t = t->right;
	while (t->node == AST_AND) {
		t2 = t->left;
		s2 = bior_expr2ic(t2);
		emit_ic(IC_CMP_0 + i32, cast(s2, dt), NULL);
		emit_branch(IC_JE, lab);
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = bior_expr2ic(t);
	emit_ic(IC_CMP_0 + i32, cast(s2, dt), NULL);
	emit_branch(IC_JE, lab);
	/**
		movl 1, val
	lab:
		movl 0, val
	*/
	val = gen_vit_reg(int_type);
	emit_ic(IC_MOV + i32, val, cnst_one);
	new_label(lab);
	emit_ic(IC_MOV + i32, val, cnst_zero);

	return val;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static symbol *or_expr2ic(ast *t) {
	ast *t1, *t2;
	symbol *s1, *s2, *val, *lab;
	type *dt = int_type;

	if (t->node != AST_OR)
		/* less precedent expression*/
		return and_expr2ic(t);
	// branch label
	lab = gen_label(NULL);
	// s1 = expr1
	t1 = t->left;
	s1 = and_expr2ic(t1);
	emit_ic(IC_CMP_0 + i32, s1, NULL);
	emit_branch(IC_JNE, lab);
	// more than one operator of equal precedence
	t = t->right;
	while (t->node == AST_OR) {
		t2 = t->left;
		s2 = and_expr2ic(t2);
		emit_ic(IC_CMP_0 + i32, cast(s2, dt), NULL);
		emit_branch(IC_JNE, lab);
		t = t->right;
	}
	// last sub-expression or only has one operator(etc, a + b)
	s2 = and_expr2ic(t);
	emit_ic(IC_CMP_0 + i32, cast(s2, dt), NULL);
	emit_branch(IC_JNE, lab);
	/**
		movl 0, val
	lab:
		movl 1, val
	*/
	val = gen_vit_reg(int_type);
	emit_ic(IC_MOV + i32, val, cnst_zero);
	new_label(lab);
	emit_ic(IC_MOV + i32, val, cnst_one);

	return val;
}

/*===--------------------------------------------------------------------------
generate icode that should affect the flag register
---------------------------------------------------------------------------===*/
static void expr2ic_psw(ast *t1, symbol *s2, symbol *true_label, symbol *false_label) {
	symbol *s1;
	type *dt;

	s1 = expr2ic(t1->expr[0]);
	dt = s1->ty;
	assert(dt);
	if (s2->is_immediate && s2->u.cnst.i == 0)
		emit_ic(IC_CMP_0 + gen_post(dt->cat, dt->cat), s1, NULL);
	else if (s2->is_immediate && s2->u.cnst.i == 1)
		emit_ic(IC_CMP_1 + gen_post(dt->cat, dt->cat), s1, NULL);
	else
		emit_ic(IC_CMP + gen_post(dt->cat, dt->cat), s1, cast(s2, dt));
	if (true_label && false_label == NULL)
		emit_branch(IC_JNE, true_label);
	else if (true_label == NULL && false_label)
		emit_branch(IC_JE, false_label);
	else
		assert(0);

	return;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static symbol *cond2ic(ast *t1) {
	symbol *cond, *s1, *s2, *val, *lab;
	type *dt = t1->ty;
	
	if (t1->right && t1->right->node == AST_COND)
		cond = or_expr2ic(t1->cond);
	else
		return or_expr2ic(t1);
	s1 = expr2ic(t1->left);
	s2 = cond2ic(t1->right);
	assert(cond->ty == int_type);
	emit_ic(IC_CMP_0 + i32, cond, NULL);
	lab = gen_label(NULL);
	emit_branch(IC_JE, lab);
	val = gen_vit_reg(dt);
	emit_ic(IC_MOV + gen_post(dt->cat, dt->cat), val, cast(s1, dt));
	new_label(lab);
	emit_ic(IC_MOV + gen_post(dt->cat, dt->cat), val, cast(s2, dt));

	return val;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static symbol *asgn2ic(ast *t1) {
	symbol *s1, *s2, *val;
	type *ty = t1->ty;
	int post;

	if (is_ast_asgn(t1->node)) {
		assert(t1->right);
		s2 = asgn2ic(t1->right);
	} else
		/* evaluate the rvalue*/
		return cond2ic(t1);
	post = gen_post(ty->cat, ty->cat);
	/* need cast ?*/
	s1 = unary2ic(t1->left);
	assert(s1->ty->cat == ty->cat);
	if (t1->node != AST_ASGN) {
		// compound assignment
		val = gen_vit_reg(ty);
		emit_ic(IC_MOV + post, val, s1);
		emit_ic(eval_icode_node(t1->node) + post, val, cast(s2, ty));
		emit_ic(IC_MOV + post, s1, val);
	} else {
		// simple assignment, two structs
		if (ty->cat == CAT_STRUCT || ty->cat == CAT_UNION)
			emit_ic(IC_MOV_BLOCK, s1, s2);
		else
			emit_ic(IC_MOV + post, s1, cast(s2, ty));
	}

	return s1;
}

/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static symbol *expr2ic(ast *t1) {
	symbol *result;

	/* comma expression*/
	while (t1) {
		assert(t1->node == AST_COMMA);
		result = asgn2ic(t1->left);
		t1 = t1->right;
	}

	return result;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static void stmt2ic(ast *tree) {
	ast *t1 = tree;

	if (tree == NULL)
		return;
	/*===-------------------- now the hard work begin-------------------------
	statement =>
		labeled-statement
		compound-statement
		expression-statement
		selection-statement
		iteration-statement
		jump-statement
	---------------------------------------------------------------------===*/
	/*==-- deal with all the kinds of AST of all the statement--==*/
	switch (t1->node) {
	/* ==--statement--==*/
	case AST_BLOCK:
		block2ic(t1);
		break;
	case AST_IF:
		/*==-------------------------------------------------------
		if (expr) statement1
		else statement2
		===--->
			if (expr == 0) goto L
			statement1
			goto L+1
		L:	statemetn2
		L+1:
		-------------------------------------------------------==*/
		{
			symbol *lab0, *lab1;
			lab0 = gen_label(NULL);
			lab1 = gen_label(NULL);
			
			expr2ic_psw(t1->expr[0], cnst_zero, NULL, lab0);
			stmt2ic(t1->left);
			//emit_ic(IC_JMP, NULL, NULL, lab1);
			emit_branch(IC_JMP, lab1);
			emit_ic(IC_LABEL, lab0, NULL);
			/* if has no else statement, peephole optimzation can delete the redundant jmp*/
			stmt2ic(t1->right);
			emit_ic(IC_LABEL, lab1, NULL);
		}
		break;
	case AST_DO:
		/*==-------------------------------------------------------
		do statement
		while (expr);
		===--->	
		L:	statement
		L+1:if (expr != 0) goto L
		L+2:
		-------------------------------------------------------==*/
		{
			symbol *lab0 = gen_label(NULL);
			symbol *lab1 = gen_label(NULL);
			symbol *lab2 = gen_label(NULL);

			t1->bk_label = lab2;
			t1->ct_label = lab1;
			emit_ic(IC_LABEL, lab0, NULL);
			stmt2ic(t1->right);
			emit_ic(IC_LABEL, lab1, NULL);
			expr2ic_psw(t1->expr[0], cnst_zero, lab0, NULL);
			emit_ic(IC_LABEL, lab2, NULL);
		}
		break;
	case AST_FOR:
		/*==-------------------------------------------------------
		for (expr1; exor2; expr3) statement
		===--->
			expr1
			goto L+2
		L:	statement
		L+1:expr3
		L+2:if (expr2 != 0) goto L
		L+3:
		-------------------------------------------------------==*/
		{
			symbol *lab0 = gen_label(NULL);
			symbol *lab1 = gen_label(NULL);
			symbol *lab2 = gen_label(NULL);
			symbol *lab3 = gen_label(NULL);

			t1->bk_label = lab3;
			/* attention this*/
			t1->ct_label = lab1;
			expr2ic(t1->expr[0]);
			//emit_ic(IC_JMP, NULL, NULL, lab2);
			emit_branch(IC_JMP, lab2);
			emit_ic(IC_LABEL, lab0, NULL);
			stmt2ic(t1->right);
			emit_ic(IC_LABEL, lab1, NULL);
			expr2ic(t1->expr[2]);
			emit_ic(IC_LABEL, lab2, NULL);
			expr2ic_psw(t1->expr[1], cnst_zero, lab0, NULL);
			emit_ic(IC_LABEL, lab3, NULL);
		}
		break;
	case AST_WHILE:
		/*==-------------------------------------------------------
		while (expr)
		statement
		===--->
			goto L+1
		L:	statement
		L+1:if (expr != 0) goto L
		L+2:
		-------------------------------------------------------==*/
		{
			symbol *lab0 = gen_label(NULL);
			symbol *lab1 = gen_label(NULL);
			symbol *lab2 = gen_label(NULL);

			t1->bk_label = lab2;
			t1->ct_label = lab1;
			//emit_ic(IC_JMP, NULL, NULL, lab1);
			emit_branch(IC_JMP, lab1);
			emit_ic(IC_LABEL, lab0, NULL);
			stmt2ic(t1->right);
			emit_ic(IC_LABEL, lab1, NULL);
			expr2ic_psw(t1->expr[0], cnst_zero, lab0, NULL);
			emit_ic(IC_LABEL, lab2, NULL);
		}
		break;
	case AST_SWITCH:
		/*==-------------------------------------------------------
		switch (expr)
		case c1: statement1;
		case c2: statement2;
		case c3: statement3;
		default: statement
		===--->
			t1 = expr
		L+1:if(t1 != c1)
			goto L+2
			statement1
		L+2:if(t1 != c2)
			goto L+3
			statement2
		L+3:if(t1 != c3)
			goto def_lab
			statement1
		def_lab:
			statement
		lab_break:
		-------------------------------------------------------==*/
		{
			symbol *cur_lab, *next_lab;
			ast *it;// *case_val;
			dlist *idx = t1->cases;
			ast *def = t1->def;

			/* icode time*/
			cur_lab = gen_label(NULL);
			for (; idx; idx = idx->next) {
				it = (ast *)idx->it;
				next_lab = gen_label(NULL);
				//case_val = new_node(AST_CNST, NULL, NULL);
				//case_val->u.cnst = it->u.cnst;
				new_label(cur_lab);
				expr2ic_psw(t1->expr[0], cnst_zero, next_lab, NULL);
				//s1 = expr2ic(t1->expr[0]);
				//emit_ic(IC_CMP + i32, cast(s1, int_type), it->u.cnst);
				//emit_branch(IC_JNE, next_lab);
				stmt2ic(it->right);
				cur_lab = next_lab;
			}
			/* default statement*/
			if (def) {
				new_label(next_lab);
				stmt2ic(def->right);
				next_lab = gen_label(NULL);
			} else
				t1->bk_label = next_lab;
		}

	case AST_GOTO:
		//fold_labels(left);
		//emit_ic(IC_JMP, NULL, NULL, (symbol *)t1->dst);
		emit_branch(IC_JMP, (symbol *)t1->dst);
		break;
	case AST_BREAK:
		assert(t1->bk_label);
		//emit_ic(IC_JMP, NULL, NULL, t1->bk_label);
		emit_branch(IC_JMP, t1->bk_label);
		break;
	case AST_CONTINUE:
		assert(t1->ct_label);
		//emit_ic(IC_JMP, NULL, NULL, t1->ct_label);
		emit_branch(IC_JMP, t1->ct_label);
		break;
	case AST_RETURN:
		//ret_stmt2icode(t1);
		{
			symbol *s1 = expr2ic(t1->right);
			//type *t1 = cur_func->ty;
			s1 = cast(s1, _F.cur_func->ty);
			emit_ic(IC_FUNC_END, s1, NULL);
			//emit_ic(IC_JMP, NULL, NULL, _F.cur_func_end);
			emit_branch(IC_JMP, _F.cur_func_end);
		}
		break;
	case AST_CASE:
	case AST_DEFAULT:
		/* have be processed, just pass it*/
		break;
	/* ==--expression--==*/
	default:
		expr2ic(t1);
	}

	return;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static void stmt_list2ic(ast *tree) {
	ast *t1 = tree;
	/*==-- statement-list, for every statement--==*/
	while (t1) {
		assert(t1->node == AST_LIST);
		stmt2ic(t1->left);
		/* next statement of statement-list*/
		t1 = t1->right;
	}

	return;
}

/*===------------------------------------------------------------------------

------------------------------------------------------------------------===*/
static void block2ic(ast *t) {
	assert(t->node = AST_BLOCK);
	stmt_list2ic(t->right);
	
	return;
}

/*===-----------------------------------------------------------------------

-----------------------------------------------------------------------===*/
void init_cnst_ast() {
	value v;

	/* initialize the constant AST*/
	ast_cnst_zero = new_node(AST_CNST, NULL, NULL);
	ast_cnst_zero->ty = int_type;
	v.i = 0;
	ast_cnst_zero->u.cnst = cnst_zero = constant(v, int_type);
	ast_cnst_one = new_node(AST_CNST, NULL, NULL);
	ast_cnst_one->ty = int_type;
	v.i = 1;
	ast_cnst_one->u.cnst = cnst_one = constant(v, int_type);

	return;
}

/*===-----------------------------------------------------------------------

-----------------------------------------------------------------------===*/
static void izs2ic_local() {
	ast *iz, *t, *t1;
	slist *p = _F.stack_inits;
	symbol *s, *val;
	int cat;

	for (; p; p = p->next) {
		s = (symbol *)p->it;
		assert(s->scope > GLOBAL);
		iz = s->iz;
		assert(iz);
		cat = s->ty->cat;
		// simple initializer
		if (is_scalar_cat(cat)) {
			if (iz->node == AST_INIT_LIST)
				error(-1, iz->line_no, "illegal initializer\n");
			else {
				t1 = new_node(AST_ID, NULL, NULL);
				t1->u.id = s;
				t1->ty = s->ty;
				t = new_node(AST_ASGN, t1, iz);
				t->ty = t1->ty;
				asgn2ic(t);
			}
		// array, struct, union
		} else {
			switch (cat) {
			case CAT_ARRAY:
				if (iz->node == AST_INIT_LIST)
					assert(!"TODO\n");
				else {
					val = iz->u.cnst;
					assert(val->ty->cat == CAT_ARRAY);
					// complete array size
					if (s->ty->size == 0)
						s->ty->size = val->ty->size + 1;		// add tail 0 of srting
					emit_ic(IC_MOV_BLOCK, s, val);
				}
				break;
			case CAT_STRUCT:
			case CAT_UNION:
				assert(!"TODO\n");
			default:
				error(-1, iz->line_no, "illegal initializer\n");
			}
		}	
	}
}

/*===-----------------------------------------------------------------------

-----------------------------------------------------------------------===*/
void ast2icode(ast *t) {
	icode *ic1;

	/* we need know the stack size to be reserved for locals and register spills,
	when optput the asm file for IC_FUNC_BEGIN*/
	_F.cur_func_end = gen_label("end");
	NEW0(ic1, FUNC);
	ic1->node = IC_FUNC_BEGIN;
	_F.root_ic = _F.tail_ic = ic1;
	assert(t->node == AST_FUNC);
	//assert(t1->right->node == AST_BLOCK);
	/* local initializers should add after IC_FUNC_BEGIN immediately*/
	izs2ic_local();
	/* statement list*/
	block2ic(t->right);
	// creat function end mark
	NEW0(ic1, FUNC);
	ic1->node = IC_FUNC_END;
	add2ic_list(ic1);
	/* free memory*/
	zcc_free(AST);

	return;
}
