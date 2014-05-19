/**
Copyright (C) 2013
	Writed by zet
*/

#include "zcchdr.h"
#include "stdhdr.h"

static void check_args(dlist *params, ast *t1);
/*===-------------------------------------------------------------------------
integral promotion 
---------------------------------------------------------------------------===*/
type *raise_to_int(type *ty) {
	if (is_low_cat(ty->cat))
		return int_type;
	else
		return ty;
}

/*===-------------------------------------------------------------------------
implementation the usual arithmetic conversion not strictly, if concern the 
unsigned type, the whole code will be horrible, think about the overflow test,
boring and no interesting, let me go.
---------------------------------------------------------------------------===*/
type *get_arith_type(type *lty, type *rty) {
	type *result;
	int cat;

	assert(is_arith_cat(lty->cat));
	assert(is_arith_cat(rty->cat));
	/* CAT_POINTER == CAT_DOUBLE+1*/
	cat = lty->cat > rty->cat ? lty->cat : rty->cat;
	if (is_int_cat(cat) /*&& cat != CAT_UINT*/)
		result = int_type;
	//else if (cat == CAT_UINT)
		//result = unsigned_int;
	else if (cat == CAT_FLOAT)
		result = float_type;
	else if (cat == CAT_DOUBLE)
		result = double_type;
	else if (cat == CAT_POINTER) {		/* special case*/
		if (lty->cat == CAT_POINTER)
			result = lty;
		else {
			assert(rty->cat == CAT_POINTER);
			result = rty;
		}
	}	

	return result;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static type *imp_pointer_int(type *ty) {
	/* this test is important, make sure every expression didn't break 
	the rule about type initialize*/
	assert(ty);
	if (ty->cat == CAT_FUNCTION)
		/* this type is for AST, it must be TY but not PERM*/
		ty = pointer(ty, TY);
	else if (ty->cat == CAT_ARRAY)
		ty = pointer(ty->bty, TY);

	return ty;
}

/*===--------------------------------------------------------------------------
implementation from function and array type changned to pointer type implicitly.
and doing this for parameter (ast *t)'s sub-tree
this function can only be called in AST pass and icode pass, so we do not need
call the clone_type().
---------------------------------------------------------------------------===*/
void imp_pointer(ast *t) {
	assert(t);

	if (t->left)
		t->left->ty = imp_pointer_int(t->left->ty);
	else if (t->right)
		t->right->ty = imp_pointer_int(t->right->ty);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static int is_lvalue(ast *tree) {
	int node = tree->node;
	ast *t1 = tree;
	type *ty = t1->ty;

	switch (node) {
	case AST_ID:
		if (ty->cat != CAT_ARRAY && ty->cat != CAT_FUNCTION)
			return 1;
	case AST_IND:
	case AST_ARROW:
	case AST_MEMBER:
			return 1;
	}

	return 0;
}

/*===-------------------------------------------------------------------------
if the root of the ast tree is a modifiable lvalue return 1, otherwise return 0
---------------------------------------------------------------------------===*/
static int is_mdf_lvalue(ast *t) {
	if (is_lvalue(t) && t->ty->is_const == 0)
		return 1;
	return 0;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void need_lvalue(ast *t1) {
	if (is_lvalue(t1) == 0)
		error(-1, t1->line_no, "need a lvalue\n");
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void need_mdf_lvalue(ast *t1) {
	if (is_mdf_lvalue(t1) == 0)
		error(-1, t1->line_no, "need a modifiable lvalue\n");
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void need_type(ast *t, int cat) {
	int kind;
	assert(t && t->ty);
	kind = t->ty->cat;
	
	switch (cat) {
	case CAT_INT:
		if (is_int_cat(kind))
			return;
		else
			error(-1, t->line_no, "need integral type value\n");
		break;
	case CAT_POINTER:
		if(kind != CAT_POINTER)
			error(-1, t->line_no, "need pointer type\n");

	/* this is special*/
	case CAT_ARITH:
		if (is_arith_cat(kind))
			return;
		else
			error(-1, t->line_no, "need arithmetic type\n");
	case CAT_SCALAR:
		if (is_scalar_cat(kind))
			return;
		else
			error(-1, t->line_no, "need scalar type\n");
	case CAT_SU:
		if (cat == CAT_STRUCT || cat == CAT_UNION)
			return;
		else
			error(-1, t->line_no, "need struct/union type\n");
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void need_no_null(void *p, int line, char *s) {
	if (p == NULL)
		error(-1, line, s);
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void fold_unary_cnst(int op, ast *t) {
	assert(t && t->node == AST_CNST);
	assert(t->right->ty);
	t->ty = raise_to_int(t->right->ty);
	switch (op) {
	case '+':
		/* nothing need to do*/
		break;
	case '-':
		/* must be arithmetic type*/
		if (t->ty->cat == CAT_INT || t->ty->cat == CAT_UINT)
			t->u.cnst->u.cnst.i = -(t->u.cnst->u.cnst.i);
		else if (t->ty->cat == CAT_FLOAT)
			t->u.cnst->u.cnst.f = -(t->u.cnst->u.cnst.f);
		else if (t->ty->cat == CAT_DOUBLE)
			t->u.cnst->u.cnst.d = -(t->u.cnst->u.cnst.d);
		else
			assert(0);
	case '~':
		/* must be integral type*/
		if (t->ty->cat == CAT_INT || t->ty->cat == CAT_UINT)
			t->u.cnst->u.cnst.i = ~(t->u.cnst->u.cnst.i);
		else
			assert(0);
	case '!':
		/* must be scalar type*/
		switch (t->ty->cat) {
		case CAT_INT:
		case CAT_UINT:
			t->u.cnst->u.cnst.i = !(t->u.cnst->u.cnst.i);
		case CAT_FLOAT:
			t->u.cnst->u.cnst.i = !(t->u.cnst->u.cnst.f);
			t->ty = int_type;
		case CAT_DOUBLE:
			t->u.cnst->u.cnst.i = !(t->u.cnst->u.cnst.d);
			t->ty = int_type;
		case CAT_POINTER:
			t->u.cnst->u.cnst.i = !(t->u.cnst->u.cnst.p);
			t->ty = int_type;
		}
	}

	return;
}
#if 0
//enum {rank_nop, rank_add, rank_mul, rank_last};
/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static int get_rank(ast *t) {
	switch (t->node) {
	case AST_MUL:
	case AST_DIV:
	case AST_MOD:
		return AST_MUL;
	}
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void reorder_ast(ast **tt) {
	int rank;
	ast *p = *tt, *tmp, *dst, *src, **foot;

	rank = get_rank(*tt);
	/* attention: this function is related with AST shape very close,
	the operand of binary operator is it's left sub-tree, expect first one*/
	foot = tt;
	for (; rank == get_rank((*foot)->left); foot = &(*foot)->left)
		;

}
#endif
/*===-------------------------------------------------------------------------
evalute two constant AST, return the result's type
---------------------------------------------------------------------------===*/
static ast *eval_cnst_ast(int op, ast *left, ast *right) {
	int lc, rc;
	ast *result;
	type *ty;
	value v = {0};

	assert(left->node == AST_CNST);
	assert(right->node == AST_CNST);
	lc = left->ty->cat;
	rc = right->ty->cat;
	result = new_node(AST_CNST, NULL, NULL);
	result->ty = ty = get_arith_type(left->ty, right->ty);
/**
	if (op == AST_G) {
		op = AST_L;
		tmp = left;
		left = right;
		right = tmp;
	} else if (op == AST_GE) {
		op = AST_LE;
		tmp = left;
		left = right;
		right = tmp;
	}
*/
	switch (op) {
	/*==------------------------------ * / % ------------------------------==*/
	case AST_MUL:
		/* the * operator*/
		switch (ty->cat) {
		case CAT_INT:
			v.i = left->u.cnst->u.cnst.i * right->u.cnst->u.cnst.i;
			break;
		case CAT_UINT:
			/* Attention: the object store the result is int type, 
			but the ast->ty is unsigned int, this right*/
			v.i = (unsigned int)left->u.cnst->u.cnst.i * (unsigned int)right->u.cnst->u.cnst.i;
			break;
		case CAT_FLOAT:
			/* [int, float] * [int, float]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i * (float)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i * right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.f = left->u.cnst->u.cnst.f * (float)right->u.cnst->u.cnst.i;
			else
				v.f = left->u.cnst->u.cnst.f * right->u.cnst->u.cnst.f;
			break;
		case CAT_DOUBLE:
			/* [int, float, double] * [int, float, double]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i * (double)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i * (double)right->u.cnst->u.cnst.f;
			else if (is_int_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i * right->u.cnst->u.cnst.d;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f * (double)right->u.cnst->u.cnst.i;
			else if (is_float_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f * (double)right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f * right->u.cnst->u.cnst.d;
			else if (is_double_cat(lc) && is_int_cat(rc))
				v.d = left->u.cnst->u.cnst.d * (double)right->u.cnst->u.cnst.i;
			else if (is_double_cat(lc) && is_float_cat(rc))
				v.d = left->u.cnst->u.cnst.d * (double)right->u.cnst->u.cnst.f;
			else
				v.d = left->u.cnst->u.cnst.d * right->u.cnst->u.cnst.d;
			break;
		default:
			assert(0);
		}
	case AST_DIV:
		/* the / operator*/
		switch (ty->cat) {
		case CAT_INT:
			v.i = left->u.cnst->u.cnst.i / right->u.cnst->u.cnst.i;
			break;
		case CAT_UINT:
			/* Attention: the object store the result is int type, 
			but the ast->ty is unsigned int, this right*/
			v.i = (unsigned int)left->u.cnst->u.cnst.i / (unsigned int)right->u.cnst->u.cnst.i;
			break;
		case CAT_FLOAT:
			/* [int, float] * [int, float]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i / (float)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i / right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.f = left->u.cnst->u.cnst.f / (float)right->u.cnst->u.cnst.i;
			else
				v.f = left->u.cnst->u.cnst.f / right->u.cnst->u.cnst.f;
			break;
		case CAT_DOUBLE:
			/* [int, float, double] * [int, float, double]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i / (double)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i / (double)right->u.cnst->u.cnst.f;
			else if (is_int_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i / right->u.cnst->u.cnst.d;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f / (double)right->u.cnst->u.cnst.i;
			else if (is_float_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f / (double)right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f / right->u.cnst->u.cnst.d;
			else if (is_double_cat(lc) && is_int_cat(rc))
				v.d = left->u.cnst->u.cnst.d / (double)right->u.cnst->u.cnst.i;
			else if (is_double_cat(lc) && is_float_cat(rc))
				v.d = left->u.cnst->u.cnst.d / (double)right->u.cnst->u.cnst.f;
			else
				v.d = left->u.cnst->u.cnst.d / right->u.cnst->u.cnst.d;
			break;
		default:
			assert(0);
		}
	case AST_MOD:
		/* the % operator*/
		switch (ty->cat) {
		case CAT_INT:
		case CAT_UINT:
			v.i = left->u.cnst->u.cnst.i % right->u.cnst->u.cnst.i;
			break;
		default:
			assert(0);
		}
	/*==------------------------------ + & - ------------------------------==*/
	case AST_ADD:
		/* the + operator*/
		switch (ty->cat) {
		case CAT_INT:
		case CAT_UINT:
			v.i = left->u.cnst->u.cnst.i + right->u.cnst->u.cnst.i;
			break;
		case CAT_FLOAT:
			/* [int, float] * [int, float]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i + (float)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i + right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.f = left->u.cnst->u.cnst.f + (float)right->u.cnst->u.cnst.i;
			else
				v.f = left->u.cnst->u.cnst.f + right->u.cnst->u.cnst.f;
			break;
		case CAT_DOUBLE:
			/* [int, float, double] * [int, float, double]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i + (double)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i + (double)right->u.cnst->u.cnst.f;
			else if (is_int_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i + right->u.cnst->u.cnst.d;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f + (double)right->u.cnst->u.cnst.i;
			else if (is_float_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f + (double)right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f + right->u.cnst->u.cnst.d;
			else if (is_double_cat(lc) && is_int_cat(rc))
				v.d = left->u.cnst->u.cnst.d + (double)right->u.cnst->u.cnst.i;
			else if (is_double_cat(lc) && is_float_cat(rc))
				v.d = left->u.cnst->u.cnst.d + (double)right->u.cnst->u.cnst.f;
			else
				v.d = left->u.cnst->u.cnst.d + right->u.cnst->u.cnst.d;
			break;
		case CAT_POINTER:
			break;
		default:
			assert(0);
		}
	case AST_SUB:
		/* the - operator*/
		switch (ty->cat) {
		case CAT_INT:
		case CAT_UINT:
			v.i = left->u.cnst->u.cnst.i - right->u.cnst->u.cnst.i;
			break;
		case CAT_FLOAT:
			/* [int, float] * [int, float]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i - (float)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.f = (float)left->u.cnst->u.cnst.i - right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.f = left->u.cnst->u.cnst.f - (float)right->u.cnst->u.cnst.i;
			else
				v.f = left->u.cnst->u.cnst.f - right->u.cnst->u.cnst.f;
			break;
		case CAT_DOUBLE:
			/* [int, float, double] * [int, float, double]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i - (double)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i - (double)right->u.cnst->u.cnst.f;
			else if (is_int_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.i - right->u.cnst->u.cnst.d;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f - (double)right->u.cnst->u.cnst.i;
			else if (is_float_cat(lc) && is_float_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f - (double)right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_double_cat(rc))
				v.d = (double)left->u.cnst->u.cnst.f - right->u.cnst->u.cnst.d;
			else if (is_double_cat(lc) && is_int_cat(rc))
				v.d = left->u.cnst->u.cnst.d - (double)right->u.cnst->u.cnst.i;
			else if (is_double_cat(lc) && is_float_cat(rc))
				v.d = left->u.cnst->u.cnst.d - (double)right->u.cnst->u.cnst.f;
			else
				v.d = left->u.cnst->u.cnst.d - right->u.cnst->u.cnst.d;
			break;
		case CAT_POINTER:
			break;
		default:
			assert(0);
		}
		/*==------------------------------ shift_op ------------------------------==*/
		case AST_LS:
		/* the << operator*/
		switch (ty->cat) {
		case CAT_INT:
		case CAT_UINT:
			v.i = left->u.cnst->u.cnst.i << right->u.cnst->u.cnst.i;
			break;
		default:
			assert(0);
		}
		case AST_RS:
		/* the >> operator*/
		switch (ty->cat) {
		case CAT_INT:
			v.i = left->u.cnst->u.cnst.i >> right->u.cnst->u.cnst.i;
			break;
		case CAT_UINT:
			/* i add the CAT_UINT is only for this*/
			v.i = (unsigned int)left->u.cnst->u.cnst.i >> right->u.cnst->u.cnst.i;
			break;
		default:
			assert(0);
		}
		/*==------------------------------ rel_op ------------------------------==*/
	case AST_L:
		/* the < operator*/
		switch (ty->cat) {
		case CAT_INT:
			v.i = left->u.cnst->u.cnst.i < right->u.cnst->u.cnst.i;
			break;
		case CAT_UINT:
			v.i = (unsigned int)left->u.cnst->u.cnst.i < (unsigned int)right->u.cnst->u.cnst.i;
			break;
		case CAT_FLOAT:
			/* [int, float] * [int, float]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.i = (float)left->u.cnst->u.cnst.i < (float)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.i = (float)left->u.cnst->u.cnst.i < right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.i = left->u.cnst->u.cnst.f < (float)right->u.cnst->u.cnst.i;
			else
				v.i = left->u.cnst->u.cnst.f < right->u.cnst->u.cnst.f;
			break;
		case CAT_DOUBLE:
			/* [int, float, double] * [int, float, double]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.i < (double)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.i < (double)right->u.cnst->u.cnst.f;
			else if (is_int_cat(lc) && is_double_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.i < right->u.cnst->u.cnst.d;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.f < (double)right->u.cnst->u.cnst.i;
			else if (is_float_cat(lc) && is_float_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.f < (double)right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_double_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.f < right->u.cnst->u.cnst.d;
			else if (is_double_cat(lc) && is_int_cat(rc))
				v.i = left->u.cnst->u.cnst.d < (double)right->u.cnst->u.cnst.i;
			else if (is_double_cat(lc) && is_float_cat(rc))
				v.i = left->u.cnst->u.cnst.d < (double)right->u.cnst->u.cnst.f;
			else
				v.i = left->u.cnst->u.cnst.d < right->u.cnst->u.cnst.d;
			break;
		case CAT_POINTER:
			break;
		default:
			assert(0);
		}
	case AST_LE:
		/* the <= operator*/
		switch (ty->cat) {
		case CAT_INT:
			v.i = left->u.cnst->u.cnst.i <= right->u.cnst->u.cnst.i;
			break;
		case CAT_UINT:
			v.i = (unsigned int)left->u.cnst->u.cnst.i <= (unsigned int)right->u.cnst->u.cnst.i;
			break;
		case CAT_FLOAT:
			/* [int, float] * [int, float]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.i = (float)left->u.cnst->u.cnst.i <= (float)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.i = (float)left->u.cnst->u.cnst.i <= right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.i = left->u.cnst->u.cnst.f <= (float)right->u.cnst->u.cnst.i;
			else
				v.i = left->u.cnst->u.cnst.f <= right->u.cnst->u.cnst.f;
			break;
		case CAT_DOUBLE:
			/* [int, float, double] * [int, float, double]*/
			if (is_int_cat(lc) && is_int_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.i <= (double)right->u.cnst->u.cnst.i;
			else if (is_int_cat(lc) && is_float_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.i <= (double)right->u.cnst->u.cnst.f;
			else if (is_int_cat(lc) && is_double_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.i <= right->u.cnst->u.cnst.d;
			else if (is_float_cat(lc) && is_int_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.f <= (double)right->u.cnst->u.cnst.i;
			else if (is_float_cat(lc) && is_float_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.f <= (double)right->u.cnst->u.cnst.f;
			else if (is_float_cat(lc) && is_double_cat(rc))
				v.i = (double)left->u.cnst->u.cnst.f <= right->u.cnst->u.cnst.d;
			else if (is_double_cat(lc) && is_int_cat(rc))
				v.i = left->u.cnst->u.cnst.d <= (double)right->u.cnst->u.cnst.i;
			else if (is_double_cat(lc) && is_float_cat(rc))
				v.i = left->u.cnst->u.cnst.d <= (double)right->u.cnst->u.cnst.f;
			else
				v.i = left->u.cnst->u.cnst.d <= right->u.cnst->u.cnst.d;
			break;
		case CAT_POINTER:
			break;
		default:
			assert(0);
		}
		/*==------------------------------ == & != ------------------------------==*/
	case AST_EQU:
		v.i = left->u.cnst->u.cnst.d == right->u.cnst->u.cnst.d;
		break;
	case AST_UE:
		v.i = left->u.cnst->u.cnst.d != right->u.cnst->u.cnst.d;
		break;
#if 0
	case AST_BAND:
		v.i = left->u.cnst->u.cnst.i & right->u.cnst->u.cnst.i;
		break;
	case AST_BXOR:
		v.i = left->u.cnst->u.cnst.i ^ right->u.cnst->u.cnst.i;
		break;
	case AST_BIOR:
		v.i = left->u.cnst->u.cnst.i | right->u.cnst->u.cnst.i;
		break;
	case AST_AND:
		v.i = left->u.cnst->u.cnst.d && right->u.cnst->u.cnst.d;
		break;
	case AST_OR:
		v.i = left->u.cnst->u.cnst.d || right->u.cnst->u.cnst.d;
		break;
#endif
	default:
		assert(0);
	} // switch (node)
	/* creat the content of constant AST*/
	result->u.cnst = constant(v, ty);

	return result;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
int is_in_set(int t, int set[]) {
	int *p = set;

	for (; *p && *p != t; p++)
		;
	if (*p == 0)
		return 0;
	else {
		assert(*p == t);
		return 1;
	}
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void eval_node_set(ast *t, int *set) {
	switch (t->node) {
	case AST_EQU:
	case AST_UE:
		set[0] = AST_EQU;
		set[1] = AST_UE;
		set[2] = 0;
		break;
	case AST_G:
	case AST_GE:
		assert(0);
	case AST_L:
	case AST_LE:
		set[0] = AST_L;
		set[1] = AST_LE;
		set[2] = 0;
		break;
	case AST_LS:
	case AST_RS:
		set[0] = AST_LS;
		set[1] = AST_RS;
		set[2] = 0;
		break;
	case AST_ADD:
	case AST_SUB:
		set[0] = AST_ADD;
		set[1] = AST_SUB;
		set[2] = 0;
		break;
	case AST_MUL:
	case AST_DIV:
	case AST_MOD:
		set[0] = AST_MUL;
		set[1] = AST_DIV;
		set[2] = AST_MOD;
		set[3] = 0;
		break;
	default:
		assert(0);
	}

	return;
}

/*===-------------------------------------------------------------------------
fold constant expression for binary expression
---------------------------------------------------------------------------===*/
ast *fold_cnst_bin(ast *t) {
	ast *t1 = t, *t2;
	int node = t1->node;
	int set[4] = {0, };

	eval_node_set(t1, set);
	/* do the real thing*/
	assert(is_in_set(node, set));
	while (t1->left->node == AST_CNST) {
		if (is_in_set(t1->right->node, set)) {
			// cnst == ast(cnst, ast)
			t2 = t1->left;
			t1 = t1->right;
			t1->left = eval_cnst_ast(node, t2, t1->left);
		} else if (t1->right->node == AST_CNST)
			// cnst == cnst
			t1 = eval_cnst_ast(node, t1->left, t1->right);
	}
#if 0
	while (is_in_set(node, set)) {
		if (t1->left->node == AST_CNST) {
			// cnst == cnst
			if (t1->right->node == AST_CNST) {
				t1 = eval_cnst_ast(node, t1->left, t1->right);
			// cnst == ast(cnst, ast)
			} else if (is_in_set(t1->right->node, set)
				&& t1->right->left->node == AST_CNST) {
				//assert(is_in_set(t1->right->node, set));
				t2 = t1->left;
				t1 = t1->right;
				t1->left = eval_cnst_ast(node, t2, t1->left);
			} else
				break;
		}
		node = t1->node;
	}
#endif
	return t1;
}

/*===-------------------------------------------------------------------------
short cut evalution
---------------------------------------------------------------------------===*/
ast *fold_cnst_logic(ast *t) {
	ast *t1 = t->left;

	assert(t1 && t->right);
	/* or-expr*/
	if (t->node == AST_OR) {
		if (t1->node == AST_CNST) {
			if (t1->u.cnst->u.cnst.i)
				return ast_cnst_one;
			else {
				if (t->right->node == AST_OR)
					return fold_cnst_logic(t->right);
				else
					return t->right;
			}
		}
	/* and-expr*/
	} else if (t->node == AST_AND) {
		if (t1->node == AST_CNST) {
			if (t1->u.cnst->u.cnst.i == 0)
				return ast_cnst_zero;
			else {
				if (t->right->node == AST_OR)
					return fold_cnst_logic(t->right);
				else
					return t->right;
			}
		}
	} else
		 assert(!"logical AST error");

	return t;
}
#if 0
/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void fold_cnst_bitwise(ast **tt, ast *left, ast *right) {
	ast *t1;
	int ln, rn, old_node;

	ln = left->node;
	rn = right->node;
	old_node = (*tt)->node;
	if (ln == AST_CNST && rn == AST_CNST) {
		/* cnst x cnst*/
		*tt = left;
		(*tt)->node = AST_CNST;
		eval_cnst_ast(old_node, left, left, right);
		/* attention the unsigned type, so reset the type of AST*/
		(*tt)->ty = int_type;
		return;
	}
	(*tt)->left = left;
	(*tt)->right = right;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void fold_cnst_shift(ast **tt, ast *left, ast *right) {
	ast *t1;
	int ln, rn, old_node;

	ln = left->node;
	rn = right->node;
	old_node = (*tt)->node;
	if (ln == AST_CNST && rn == AST_CNST) {
		/* (right)cnst x cnst*/
		*tt = left;
		(*tt)->node = AST_CNST;
		eval_cnst_ast(old_node, left, left, right);
		/* attention the unsigned type, so reset the type of AST*/
		(*tt)->ty = raise_to_int(left->ty);
		return;
	} else if (left->node == old_node && rn == AST_CNST
		&& left->right->node == AST_CNST) {
		/* (right)cnst x no-cnst*/
		assert(left->ty);
		*tt = left;
		/* Attention: different with fold_cnst_arith only in this line*/
		left->right->u.cnst.i += right->u.cnst.i;
		return;
	} else if (left->node == old_node && ln == AST_CNST) {
		/* no-cnst x cnst*/
		if (right->left)
			assert(right->left->node != AST_CNST);
		if (right->right)
			assert(right->right->node != AST_CNST);
		/* make sure constant in the right sub-tree*/
		t1 = left;
		left = right;
		right = t1;
	} else if (left->node == old_node && left->right->node == AST_CNST) {
		/* no-cnst x no-cnst*/
		t1 = left->right;
		left->right = right;
		right = t1;
	}
	(*tt)->left = left;
	(*tt)->right = right;

	return;
}

/*===-------------------------------------------------------------------------
reorder operands of binary operator maybe cause overflow or illegal, 
so only doing conservative constant fold
---------------------------------------------------------------------------===*/
void fold_cnst_arith(ast **tt, ast *left, ast *right) {
	ast *t1;
	int ln, rn, old_node;

	ln = left->node;
	rn = right->node;
	old_node = (*tt)->node;
	if (ln == AST_CNST && rn == AST_CNST) {
		/* (right)cnst x cnst*/
		*tt = left;
		(*tt)->node = AST_CNST;
		eval_cnst_ast(old_node, left, left, right);
		return;
	} else if (left->node == old_node && rn == AST_CNST
		&& left->right->node == AST_CNST) {
		/* (right)cnst x no-cnst*/
		*tt = left;
		/* only AST_CNST node need evaluate it's type*/
		eval_cnst_ast(old_node, left->right, left->right, right);
		return;
	} else if (left->node == old_node && ln == AST_CNST) {
		/* no-cnst x cnst*/
		if (right->left)
			assert(right->left->node != AST_CNST);
		if (right->right)
			assert(right->right->node != AST_CNST);
		/* make sure constant in the right sub-tree*/
		t1 = left;
		left = right;
		right = t1;
	} else if (left->node == old_node && left->right->node == AST_CNST) {
		/* no-cnst x no-cnst*/
		t1 = left->right;
		left->right = right;
		right = t1;
	}
	(*tt)->left = left;
	(*tt)->right = right;

	return;
}
#endif
/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static type *get_member_type(type *ty, char *name, int line) {
	slist *p = NULL;
	symbol *s;

	if (ty->cat == CAT_STRUCT)
		p = ty->u.s.sl;
	else
		p = ty->u.u.ul;
	assert(p);
	for (; p; p = p->next) {
		s = (symbol *)p->it;
		if (name == s->name)
			return s->ty;
	}

	error(-1, line, "member %s did not find\n", name);
	//return NULL;
}

/*===-------------------------------------------------------------------------
doing the semantic constraints check and type propagate for postfix-expression
---------------------------------------------------------------------------===*/
void check_post(ast *t) {
	ast *left, *right;
	type *lty, *rty;
	//symbol *sm = NULL;
	//all the function should check its argument has value
	assert(t);
	left = t->left;
	right = t->right;
	lty = left->ty;
	rty = right->ty;
	/* there is no '&' operator in the post-expression, 
	so we can call implicit convert outside*/
	imp_pointer(t);
	switch (t->node) {
	case AST_ARRAY:
		//imp_pointer(t1);
		need_no_null(left, t->line_no, "array name empty\n");
		need_no_null(right, t->line_no, "array index empty\n");
		need_type(left, CAT_POINTER);
		need_type(right, CAT_INT);
		/* this type maybe array type too*/
		t->ty = left->ty->bty;
		break;
	case AST_CALL:
		need_no_null(left, t->line_no, "need function name\n");
		need_type(left, CAT_POINTER);
		if (left->ty->bty->cat != CAT_FUNCTION)
			error(-1, t->line_no, "type error\n");
		check_args(left->ty->bty->u.f.params_decl, right);
		/* has return type of the function*/
		t->ty = left->ty->bty->bty;
		break;
	case AST_MEMBER:
		need_no_null(left, t->line_no, "need left operand\n");
		need_no_null(right, t->line_no, "need memeber name\n");
		need_type(left, CAT_SU);
		t->ty = get_member_type(left->ty, right->u.id->name, t->line_no);
		break;
	case AST_ARROW:
		need_no_null(left, t->line_no, "need left operand\n");
		need_no_null(right, t->line_no, "need memeber name\n");
		need_type(left, CAT_POINTER);
		if (left->ty->bty->cat == CAT_STRUCT || left->ty->bty->cat == CAT_UNION)
			t->ty = get_member_type(left->ty, right->u.id->name, t->line_no);
		else
			error(-1, t->line_no, "type error\n");
		break;
	case AST_POST_INC:
	case AST_POST_DEC:
		need_no_null(left, t->line_no, "need operand\n");
		need_type(left, CAT_SCALAR);
		need_mdf_lvalue(left);
		/* propagate type*/
		t->ty = get_arith_type(left->ty, left->ty);
		break;
	default:
		assert(0);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void id_to_ind(ast *t) {
	ast *t1 = t;
	symbol *sym = t1->u.id;

	assert(t1);
	t1->node = AST_IND;
	t1->ty = sym->ty;
	t1->right = new_node(AST_ADDR, NULL, NULL);
	t1->right->u.id = t1->u.id;
	t1->right->ty = pointer(sym->ty, TY);

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_unary(ast *t) {
	ast *t1, *right;
	type *rty;
	int cat;

	assert(t);
	t1 = t;
	right = t->right;
	rty = right->ty;
	assert(rty);
	cat = rty->cat;
	if (t1->node != AST_ADDR)
		imp_pointer(t1);
	switch (t1->node) {
	case AST_PRE_INC:
	case AST_PRE_DEC:
		need_no_null(right, t->line_no, "need operand\n");
		need_type(t->right, CAT_SCALAR);
		need_mdf_lvalue(right);
		/* propagate type*/
		t1->ty = get_arith_type(rty, int_type);
		break;
	case AST_ADDR:
		need_no_null(right, t->line_no, "need operand\n");
		if (cat == CAT_FUNCTION || cat == CAT_ARRAY 
			|| is_lvalue(right) && !rty->sym->is_bitfield && !rty->sym->is_register)
			t1->ty = pointer(rty, TY);
		else
			error(-1, t->line_no, "illegal type\n");
		break;
	case AST_IND:
		need_no_null(right, t->line_no, "need operand\n");
		need_type(t->right, CAT_POINTER);
		/* this type is special, maybe is_low_int*/
		t1->ty = rty->bty;
		break;
	case AST_PST:
	case AST_NEG:
		need_no_null(right, t->line_no, "need operand\n");
		need_type(t->right, CAT_ARITH);
		t1->ty = get_arith_type(rty, int_type);
		break;
	case AST_COM:
		need_no_null(right, t->line_no, "need operand\n");
		need_type(t->right, CAT_INT);
		t1->ty = raise_to_int(rty);
		break;
	case AST_NOT:
		need_no_null(right, t->line_no, "need operand\n");
		need_type(t->right, CAT_SCALAR);
		/* it's type is surely by language specification*/
		t1->ty = int_type;
		break;
	case AST_CNST:
		break;
	default:
		assert(0);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_cast(ast *t) {
	type *ty, *rty;

	ty = t->ty;
	assert(ty);
	rty = t->right->ty;
	assert(rty);
	if (ty->cat == CAT_VOID)
		return;
	else {
		need_type(t, CAT_SCALAR);
		need_type(t->right, CAT_SCALAR);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_mul(ast *t) {
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	/* even convert to pointer still illegal*/
	imp_pointer(t);
	if (t->node == AST_MOD) {
		need_type(t->left, CAT_INT);
		need_type(t->right, CAT_INT);
	} else {
		need_type(t->left, CAT_ARITH);
		need_type(t->right, CAT_ARITH);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_add(ast *t) {
	type *lty = NULL, *rty = NULL;

	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	lty = t->left->ty;
	assert(lty);
	rty = t->right->ty;
	assert(rty);
	/* function and array type implicit convert to pointer*/
	imp_pointer(t);
	if (lty->cat == CAT_POINTER && is_int_cat(rty->cat)
		|| is_int_cat(lty->cat) && rty->cat == CAT_POINTER)
		return;
	else if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat))
		return;
	if (t->node == AST_SUB && lty->cat == CAT_POINTER 
		&& rty->cat == CAT_POINTER && is_cpt_no_qual(lty->bty, rty->bty))
		return;

	error(-1, t->line_no, "need arithmetic or pointer\n");
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_shift(ast *t) {
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	imp_pointer(t);
	need_type(t->left, CAT_INT);
	need_type(t->right, CAT_INT);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_rel(ast *t) {
	type *lty = NULL, *rty = NULL;
	ast *t1;
	int node = 0;
	
	/* change > and >= to < and <=*/
	if (t->node == AST_G)
		node = AST_L;
	else if (t->node == AST_GE)
		node = AST_LE;
	if (node) {
		t->node = node;
		t1 = t->left;
		t->left = t->right;
		t->right = t1;
	}
	// check type
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	lty = t->left->ty;
	assert(lty);
	rty = t->right->ty;
	assert(rty);
	imp_pointer(t);
	if (lty->cat == CAT_POINTER && rty->cat == CAT_POINTER 
		&& is_cpt_no_qual(lty->bty, rty->bty))
			return;
	else if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat))
		return;
	else
		error(-1, t->line_no, "need pointer or arithmetic");

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_equ(ast *t) {
	type *lty = NULL, *rty = NULL;

	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	lty = t->left->ty;
	assert(lty);
	rty = t->right->ty;
	assert(rty);
	imp_pointer(t);
	if (lty->cat == CAT_POINTER && rty->cat == CAT_POINTER) {
		if (lty->bty->cat == CAT_VOID || rty->bty->cat == CAT_VOID)
			return;
		else if (t->left->node == AST_CNST && t->left->u.cnst->u.cnst.i == 0
				||t->right->node == AST_CNST && t->right->u.cnst->u.cnst.i == 0)
			return;
		else if (is_cpt_no_qual(lty->bty, rty->bty))
			return;
	} else if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat))
		return;
	else
		error(-1, t->line_no, "need compatiable pointer or arithmetic type");

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_bit_op(ast *t) {
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	imp_pointer(t);
	need_type(t->left, CAT_INT);
	need_type(t->right, CAT_INT);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_and(ast *t) {
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	imp_pointer(t);
	need_type(t->left, CAT_SCALAR);
	need_type(t->right, CAT_SCALAR);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_cond(ast *t) {
	type *lty = NULL, *rty = NULL;

	need_no_null(t->cond, t->line_no, "need condition expression\n");
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	lty = t->left->ty;
	assert(lty);
	rty = t->right->ty;
	assert(rty);
	t->cond->ty = imp_pointer_int(t->cond->ty);
	imp_pointer(t);
	need_type(t->cond, CAT_SCALAR);
	if (lty->cat == rty->cat) {
		// void x void
		if (lty->cat == CAT_VOID && rty->cat == CAT_VOID)
			t->ty = void_type;
		// pointer x pointer
		else if (lty->cat == CAT_POINTER) {
			// pointer x NULL || NULL x pointer
			if (t->left->node == AST_CNST && t->left->u.cnst->u.cnst.i == 0
				||t->right->node == AST_CNST && t->right->u.cnst->u.cnst.i == 0)
				// TODO: i need more time
				assert(0);
			else if (is_cpt_no_qual(lty->bty, rty->bty))
				// TODO: need evalute composite type
				t->ty = lty;
			// pointer(object) x pointer(void)
			else if (lty->bty->cat == CAT_VOID || rty->bty->cat == CAT_VOID)
				// TODO:
				assert(0);
		// struct x struct
		} else if (lty->cat == CAT_STRUCT || lty->cat == CAT_UNION) {
			if (is_cpt_no_qual(lty, rty))
				t->ty = lty;
		// arithmetic x arithmetic && left cat == right cat
		} else if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat))
			t->ty = get_arith_type(lty, rty);
	//  arithmetic x arithmetic && left cat != right cat
	} else if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat))
		t->ty = get_arith_type(lty, rty);
	else
		error(-1, t->line_no, "need compatiable pointer or arithmetic type");

	assert(t->ty);
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static int is_cpt_ptr_asgn(type *lty, ast *rt) {
	type *lbty, *rbty;

	if (lty->cat == CAT_POINTER && rt->ty->cat == CAT_POINTER) {
		lbty = lty->bty;
		rbty = rt->ty->bty;
		assert(lbty && rbty);
		/* pointer x null_pointer*/
		if (rt->node == AST_CNST && rt->u.cnst->u.cnst.i == 0)
			return 1;
		/* the type pointed to by the left has all the qulalifiers 
		of the type pointed to by the right*/
		if (! (rbty->is_const && lbty->is_const == 0
			|| rbty->is_volatile && lbty->is_volatile == 0)) {
			/* pointer(object) x pointer(object)*/
			/* pointer(object) x pointer(void) or pointer(void) x pointer(object)*/
			if (is_cpt_no_qual(lbty, rbty) || lbty->cat == CAT_VOID
				|| rbty->cat == CAT_VOID)
				return 1;
		}
	}

	return 0;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void check_asgn(ast *t) {
	type *lty = NULL, *rty = NULL;

	assert(t->node >= AST_MUL_ASGN && t->node <= AST_BIOR_ASGN \
		|| t->node == AST_ASGN);
	need_no_null(t->left, t->line_no, "need two operand\n");
	need_no_null(t->right, t->line_no, "need two operand\n");
	lty = t->left->ty;
	assert(lty);
	rty = t->right->ty;
	assert(rty);
	imp_pointer(t);
	need_mdf_lvalue(t->left);
	/* only rvalue's reference times, cut off pure lvalue*/
	//if (t->node == AST_ASGN && t->left->node == AST_ID)
		//t->left->u.id->ref_times--;
	switch (t->node) {
	case AST_ASGN:
		if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat)
		|| lty->cat == rty->cat && (lty->cat == CAT_STRUCT ||
		lty->cat == CAT_UNION) && is_cpt_no_qual(lty, rty)
		|| is_cpt_ptr_asgn(t->left->ty, t->right))
			return;
		break;
	case AST_MUL_ASGN:
	case AST_DIV_ASGN:
	case AST_MOD_ASGN:
		check_mul(t);
		break;
	case AST_ADD_ASGN:
	case AST_SUB_ASGN:
		if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat)
			|| lty->cat == CAT_POINTER && is_int_cat(rty->cat))
			return;
		break;
	case AST_LS_ASGN:
	case AST_RS_ASGN:
		check_shift(t);
		break;
	case AST_BAND_ASGN:
	case AST_BXOR_ASGN:
	case AST_BIOR_ASGN:
		check_bit_op(t);
		break;
	default:
		assert(!"Panic, AST node of assignment error");
	}

	error(-1, t->line_no, "assignement operand type illegal\n");
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static int check_simple_asgn(symbol *s1, ast *t2) {
	type *lty, *rty;
	lty = s1->ty;
	rty = t2->ty;

	if (is_arith_cat(lty->cat) && is_arith_cat(rty->cat)
		|| lty->cat == rty->cat && (lty->cat == CAT_STRUCT ||
		lty->cat == CAT_UNION) && is_cpt_no_qual(lty, rty)
		|| is_cpt_ptr_asgn(lty, t2))
			return 1;
	return 0;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void alloc_args_stack(int *stack, int *os, int size) {
	assert(size);
	assert(*os == 0 && "Panic, alloc argument stack error");
	_align(stack, size);
	*os = *stack;
	*stack += size;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static type* def_arg_promot(type *ty) {
	int cat = ty->cat;
	if (is_int_cat(cat))
		return int_type;
	else if (cat == CAT_FLOAT || cat == CAT_DOUBLE)
		return double_type;
	else
		return ty;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void check_args(dlist *params, ast *t1) {
	symbol *s1;
	ast *t2;
	type *ty1, *ty2;
	dlist *args, *d1, *d2;
	int args_offset = 0;	//esp
	int params_offset = 8;	//ebp of next frame
	/** d1 = d2*/
	d1 = params;
	d2 = args = (dlist*)t1->left;
	if (params == NULL && args || params && args == NULL)
		error(-1, t1->line_no, "function call type error\n");
	else if (params == NULL && args == NULL)
		return;
	do {
		s1 = (symbol *)d1->it;
		t2 = (ast *)d2->it;
		ty1 = imp_pointer_int(s1->ty);
		ty2 = imp_pointer_int(t2->ty);
		ty1 = raise_to_int(ty1);
		t2->ty = ty2 = raise_to_int(ty2);
		if (ty1->cat == CAT_VA)
			break;
		else if (check_simple_asgn(s1, t2)) {
			/* TODO: struct/union change to pointer implictly*/
			assert(ty1->size == ty2->size);
			alloc_args_stack(&params_offset, &s1->ebp_off, ty1->size);
			alloc_args_stack(&args_offset, &t2->u.arg_off, ty2->size);
		} else
			error(-1, t1->line_no, "argument is not compatiable with parameter\n");
		d1 = d1->next;
		d2 = d2->next;
	} while (d1 != params && d2 != args);
	/* default arguments promotions*/
	while (d2 != args) {
		t2 = (ast *)d2->it;
		ty2 = imp_pointer_int(t2->ty);
		t2->ty = def_arg_promot(ty2);
		alloc_args_stack(&args_offset, &t2->u.arg_off, ty2->size);
		d2 = d2->next;
	}
	/* updata reserved max argument size*/
	args_offset > _F.cur_args_size ? _F.cur_args_size = args_offset : 42;

	return;
}
