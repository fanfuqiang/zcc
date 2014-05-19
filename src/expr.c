/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

static ast *asgn_expr();
static ast *cast_expr();
/* first set of primary expression*/
char first_primary[] = {fsv_id, fsv_atom, fsv_lp, 0};
/*===-------------------------------------------------------------------------
	(arg1 <-> arg2 <-> arg3)
---------------------------------------------------------------------------===*/
ast *arg_expr() {
	ast *t1;
	dlist *args = NULL;

	add2dlist_tail(&args, asgn_expr(), FUNC);
	while (tk == ',')
		add2dlist_tail(&args, asgn_expr(), FUNC);
	/* hack*/
	t1 = new_node(AST_ARGS, (ast*)args, NULL);

	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *primary() {
	ast *t1;
	symbol *sym;

	expect_first_cur(first_primary);
	switch (tk) {
	case TK_ID:
		sym = look_up(ids_tb, lex_data.name);
		tk = get_tk();
		if (sym == NULL)	/* maybe this struct member, can not report error*/
			sym = gen_symbol(lex_data.name, int_type, FUNC);
		else if (sym->is_immediate) {	//enumrator
			t1 = new_node(AST_CNST, NULL, NULL);
			t1->ty = int_type;
			t1->u.cnst = sym;
			return t1;
		}
		/* will decide whether output .extern identifier*/
		sym->is_used = 1;
		/* updata symbol reference times*/
		//sym->ref_times++;
		/* after sementic analysis, there shall no AST_ID*/
		t1 = new_node(AST_ID, NULL, NULL);
		/* array and function need be convert to pointer implicitly, but not now*/
		//t1->ty = raise_to_int(sym->ty);
		t1->ty = sym->ty;
		t1->u.id = sym;
		//tk = get_tk();
		return t1;
	case TK_FCNST:
		t1 = new_node(AST_CNST, NULL, NULL);
		t1->ty = float_type;
		t1->u.cnst = lex_data.sym;
		add2slist_tail(&_F.rodata_cnsts, lex_data.sym, FUNC);
		tk = get_tk();
		return t1;
	case TK_DCNST:
		t1 = new_node(AST_CNST, NULL, NULL);
		t1->ty = double_type;
		t1->u.cnst = lex_data.sym;
		add2slist_tail(&_F.rodata_cnsts, lex_data.sym, FUNC);
		tk = get_tk();
		return t1;
	case TK_ICNST:
		t1 = new_node(AST_CNST, NULL, NULL);
		t1->ty = int_type;
		t1->u.cnst = lex_data.sym;
		tk = get_tk();
		return t1;
	case TK_SCNST:
		t1 = new_node(AST_CNST, NULL, NULL);
		t1->ty = lex_data.sym->ty;	//array(char)
		t1->u.cnst = lex_data.sym;
		//add2slist_tail(&rodata_cnsts, lex_data.sym, FUNC);
		tk = get_tk();
		return t1;
	case '(':
		tk = get_tk();
		t1 = new_node(AST_PACK_EXPR, expression(), NULL);
		assert(t1->left->ty);
		t1->ty = t1->left->ty;
		expect_tk(')');
		return t1;
	}

	error(-1, zcc_line_no, "unrecongnized characters: %s\n", g_tk2str[tk]);
}

/*===-------------------------------------------------------------------------
*postfic-expression
*	primary-expression [['['expression']'] | [(argument-list)] | [.identifier] 
*	| [->identifier] | [++] | [--]]
*	primary -> id1 -> id2  -->
*				AST_ARROW
*					/	\
*			AST_ARROW	id2	
*				/	\
*			primary	id1
*---------------------------------------------------------------------------===*/
ast *post_expr() {
	ast *t1, *t2, *t3;
	int tmp, node;

	t1 = primary();
	//tk = get_tk();
	/* attention the difference of syntax between 
	postfix-expression and unary-expression*/
	while (1)
		switch (tk) {
		case '[':
		case '(':
		case '.':
		case TK_ARROW:
			tmp = tk;
			// feed the tk
			tk = get_tk();
			if (tmp == '[') {
				node = AST_ARRAY;
				t3 = expression();
				need_no_null(t3, t3->line_no, "array index empty\n");
				expect_tk(']');
			} else if (tmp == '(') {
				node = AST_CALL;
				if (tk != ')')
					t3 = arg_expr();
				else
					t3 = NULL;
				expect_tk(')');
			} else if (tmp == '.') {
				node = AST_MEMBER;
				expect_tk_cur(TK_ID);
				t3 = primary();
			} else {
				node = AST_ARROW;
				expect_tk_cur(TK_ID);
				t3 = primary();
			}
			t2 = new_node(node, t1, t3);
			/* semantics*/
			check_post(t2);
			/* Attention the syntax rules*/
			t1 = t2;
			break;
		case TK_INC:
		case TK_DEC:
			if (tk == TK_INC)
				node = AST_POST_INC;
			else
				node = AST_POST_DEC;
			tk = get_tk();
			t2 = new_node(node, t1, NULL);
			check_post(t2);
			t1 = t2;
			break;
		default:
			goto out;
		}
out:
	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *unary() {
	ast *t1, *t2;
	char *first_unary = first_cnst;
	int node;
	int is_fold = 1;
	type *ty;
	value v;
	char *pc_tmp = get_pc_terrible();
	int tk_tmp = tk;

	expect_first_cur(first_unary);
	switch (tk) {
	case TK_INC:	
	case TK_DEC:
		if (tk == TK_INC)
			node = AST_PRE_INC;
		else
			node = AST_PRE_DEC;
		t1 = new_node(AST_PRE_INC, NULL, NULL);
		/* Attention feed*/
		tk = get_tk();
		t1->right = unary();
		check_unary(t1);
		return t1;
	case TK_SIZEOF:
		/* eat the sizeof*/
		tk = get_tk();
		/*-= sizeof (type-name)=-*/
		if (tk == '(') {
			tk = get_tk();
			if (is_first(tk, first_decl)) {
				ty = specifier(NULL);
				if (ty->size == 0)
					error(-1, zcc_line_no, "sizeof (type) -- the type can not be \
							incomplete type or function\n");
				/* constant folding*/
				t1 = new_node(AST_CNST, NULL, NULL);
				t1->ty = int_type;
				v.i = ty->size;
				t1->u.cnst = constant(v, int_type);
				// attention eat this
				expect_tk(')');
			}
		}
		/*-= sizeof unary-expression=-*/
		/* if first token after sizeof is '(' the lex-buffer may be changed, 
		so need synchronize and back.*/
		set_pc_terrible(pc_tmp);
		tk = tk_tmp;
		t2 = unary();
		if (t2->ty->size == 0)
			error(-1, t2->line_no, "sizeof unary-expression -- the type of \
					expression whose size can not be 0\n");
		/* constant folding*/
		t1 = new_node(AST_CNST, NULL, NULL);
		t1->ty = int_type;
		v.i = t2->ty->size;
		t1->u.cnst = constant(v, int_type);
		return t1;
	case '&':
	case '*':
		if (tk == '&')
			node = AST_ADDR;
		else if (tk == '*')
			node = AST_IND;
		// feed tk
		tk = get_tk();
		t2 = cast_expr();
		t1 = new_node(node, NULL, t2);
		check_unary(t1);
		return t1;
	case '+':
	case '-':
	case '~':
	case '!':
		tk_tmp = tk;
		// feed tk
		tk = get_tk();
		t2 = cast_expr();
		if (tk == '+') {
			node = AST_PST;
			if (t2->node == AST_CNST) {
				need_type(t2, CAT_ARITH);
				/* unary expression is right side combination, so recurision call unary() 
				and which has fold constant effction is correct*/
				fold_unary_cnst('+', t2);
				t1 = t2;
				is_fold = 0;
			}
		} else if (tk == '-') {
			node = AST_NEG;
			if (t2->node == AST_CNST) {
				need_type(t2, CAT_ARITH);
				fold_unary_cnst('-', t2);
				t1 = t2;
				is_fold = 0;
			}
		} else if (tk == '~') {
			node = AST_COM;
			if (t2->node == AST_CNST) {
				need_type(t2, CAT_INT);
				fold_unary_cnst('~', t2);
				t1 = t2;
				is_fold = 0;
			}
		} else {
			node = AST_NOT;
			if (t2->node == AST_CNST) {
				need_type(t2, CAT_SCALAR);
				fold_unary_cnst('!', t2);
				t1 = t2;
				is_fold = 0;
			}
		}
		if (is_fold)
			// no compile-time constant node is not AST_CNST, so build the general AST
			t1 = new_node(node, NULL, t2);
		check_unary(t1);
		return t1;
	default:
		return post_expr();
	}
	//assert(0);
}

/*===-------------------------------------------------------------------------
* cast-expr ==> '(' type-name ')' cast-expr | unary-expr
*	(type1) (type2) unary
*		AST_CAST
*		/	\
*	NULL	AST_CAST		<-- no fold constant, this struct is ok
*			/	\
*		NULL	unary
*---------------------------------------------------------------------------===*/
static ast *cast_expr() {
	ast *t1;
	type *ty1;
	char *pc_tmp = get_pc_terrible();
	int tk_tmp = tk;

	if (tk == '(') {
		tk = get_tk();
		if (is_first(tk, first_decl)) {
			ty1 = specifier(NULL);
			t1 = new_node(AST_CAST, NULL, cast_expr());
			/* this is a key point for the evaluation of whole expression system's type*/
			//t1->ty = clone_type_deep(ty1, FUNC);
			t1->ty = ty1;
			check_cast(t1);
		} else {
			set_pc_terrible(pc_tmp);
			tk = tk_tmp;
			//semantic will be check by unary() itself
			t1 = unary();
		}
	} else
		t1 = unary();
	// TODO, constant fold

	return t1;
}

/*===-------------------------------------------------------------------------
*, /, %
---------------------------------------------------------------------------===*/
ast *mul_expr() {
	ast *t1, *t2, *t3;
	int tmp, node;

	t1 = cast_expr();
	if (tk == '*' || tk == '/' || tk == '%') {
		tmp = tk;
		tk = get_tk();
		/* this function deal with == and !=*/
		if (tmp == '*')
			node = AST_MUL;
		else if (tmp == '/')
			node = AST_DIV;
		else
			node = AST_MOD;
		t1 = t2 = new_node(node, t1, cast_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_mul(t2);
		t2->ty = get_arith_type(t2->left->ty, t2->right->ty);
		while (tk == '*' || tk == '/' || tk == '%') {
			tmp = tk;
			tk = get_tk();
			/* this function deal with == and !=*/
			if (tmp == '*')
				node = AST_MUL;
			else if (tmp == '/')
				node = AST_DIV;
			else
				node = AST_MOD;
			t3 = new_node(node, t2->right, cast_expr());
			check_add(t3);
			t3->ty = get_arith_type(t3->left->ty, t3->right->ty);
			t2->right = t3;
			t2 = t3;
		}
		t1 = fold_cnst_bin(t1);
	}

	return t1;
}

/*===-------------------------------------------------------------------------
+, -
---------------------------------------------------------------------------===*/
ast *add_expr() {
	ast *t1, *t2, *t3;
	int tmp, node;

	t1 = mul_expr();
	if (tk == '+' || tk == '-') {
		tmp = tk;
		tk = get_tk();
		/* this function deal with == and !=*/
		if (tmp == '+')
			node = AST_ADD;
		else
			node = AST_SUB;
		t1 = t2 = new_node(node, t1, mul_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_add(t2);
		t2->ty = get_arith_type(t2->left->ty, t2->right->ty);
		while (tk == '+' || tk == '-') {
			tmp = tk;
			tk = get_tk();
			/* this function deal with == and !=*/
			if (tmp == '+')
				node = AST_ADD;
			else
				node = AST_SUB;
			t3 = new_node(node, t2->right, mul_expr());
			check_add(t3);
			t3->ty = get_arith_type(t3->left->ty, t3->right->ty);
			t2->right = t3;
			t2 = t3;
		}
		fold_cnst_bin(t1);
	}

	return t1;
}

/*===-------------------------------------------------------------------------
<<, >>
---------------------------------------------------------------------------===*/
ast *shift_expr() {
	ast *t1, *t2, *t3;
	int tmp, node;

	t1 = add_expr();
	if (tk == TK_LS || tk == TK_RS) {
		tmp = tk;
		tk = get_tk();
		/* this function deal with == and !=*/
		if (tmp == TK_LS)
			node = AST_LS;
		else
			node = AST_RS;
		t1 = t2 = new_node(node, t1, add_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_shift(t2);
		t2->ty = int_type;
		while (tk == TK_LS || tk == TK_RS) {
			tmp = tk;
			tk = get_tk();
			/* this function deal with == and !=*/
			if (tmp == TK_LS)
				node = AST_LS;
			else
				node = AST_RS;
			t3 = new_node(node, t2->right, add_expr());
			check_shift(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		fold_cnst_bin(t1);
	}

	return t1;
}

/*===-------------------------------------------------------------------------
>, <, <=, >=
---------------------------------------------------------------------------===*/
ast *rel_expr() {
	ast *t1, *t2, *t3;
	int tmp, node;
	char set[] = {fsv_rel_op, 0};

	t1 = shift_expr();
	if (is_first(tk, set)) {
		tmp = tk;
		tk = get_tk();
		/* deal with relational operator*/
		if (tmp == '>')
			node = AST_G;
		else if (tk == '<')
			node = AST_L;
		else if (tk == TK_GE)
			node = AST_GE;
		else
			node = AST_LE;
		t1 = t2 = new_node(node, t1, shift_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_rel(t2);
		t2->ty = int_type;
		while (is_first(tk, set)) {
			tmp = tk;
			tk = get_tk();
			/* deal with relational operator*/
			if (tmp == '>')
				node = AST_G;
			else if (tk == '<')
				node = AST_L;
			else if (tk == TK_GE)
				node = AST_GE;
			else
				node = AST_LE;
			t3 = new_node(node, t2->right, shift_expr());
			check_rel(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		fold_cnst_bin(t1);
	}

	return t1;
}

/*===-------------------------------------------------------------------------
==, !=
---------------------------------------------------------------------------===*/
ast *equ_expr() {
	ast *t1, *t2, *t3;
	int tmp, node;

	t1 = rel_expr();
	if (tk == TK_EQU || tk == TK_UE) {
		tmp = tk;
		tk = get_tk();
		/* this function deal with == and !=*/
		if (tmp == TK_EQU)
			node = AST_EQU;
		else
			node = AST_UE;
		t1 = t2 = new_node(node, t1, rel_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_equ(t2);
		t2->ty = int_type;
		while (tk == TK_EQU || tk == TK_UE) {
			tmp = tk;
			tk = get_tk();
			/* this function deal with == and !=*/
			if (tmp == TK_EQU)
				node = AST_EQU;
			else
				node = AST_UE;
			t3 = new_node(node, t2->right, rel_expr());
			check_equ(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		fold_cnst_bin(t1);
	}

	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *band_expr() {
	ast *t1, *t2, *t3;

	t1 = equ_expr();
	if (tk == '&') {
		tk = get_tk();
		t1 = t2 = new_node(AST_BAND, t1, equ_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_bit_op(t2);
		t2->ty = int_type;
		while (tk == '&') {
			tk = get_tk();
			t3 = new_node(AST_BAND, t2->right, equ_expr());
			check_bit_op(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		// TODO: fold cnst
	}

	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *bxor_expr() {
	ast *t1, *t2, *t3;

	t1 = band_expr();
	if (tk == '^') {
		tk = get_tk();
		t1 = t2 = new_node(AST_BXOR, t1, band_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_bit_op(t2);
		t2->ty = int_type;
		while (tk == '^') {
			tk = get_tk();
			t3 = new_node(AST_BXOR, t2->right, band_expr());
			check_bit_op(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		// TODO: fold cnst
	}

	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *bior_expr() {
	ast *t1, *t2, *t3;

	t1 = bxor_expr();
	if (tk == '|') {
		tk = get_tk();
		t1 = t2 = new_node(AST_BIOR, t1, bxor_expr());
		/* zcc do not generate the CVT node in this point,
		so when output the .asm file zcc need generate convert instruction*/
		check_bit_op(t2);
		t2->ty = int_type;
		while (tk == '|') {
			tk = get_tk();
			t3 = new_node(AST_BIOR, t2->right, bxor_expr());
			check_bit_op(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		// TODO: fold cnst
	}
	
	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *and_expr() {
	ast *t1, *t2, *t3;

	t1 = bior_expr();
	if (tk == TK_AND) {
		tk = get_tk();
		t1 = t2 = new_node(AST_AND, t1, bior_expr());
		check_and(t2);
		t2->ty = int_type;
		while (tk == TK_AND) {
			tk = get_tk();
			t3 = new_node(AST_AND, t2->right, bior_expr());
			check_and(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		t1 = fold_cnst_logic(t1);
	}

	return t1;
}

/*===-------------------------------------------------------------------------
*	or-expr ==> and-expr { || and-expr }
*	and-expr1 || and-expr2 || and-expr3 -->
*			AST_OR			OR		and-expr
*			/	\
*	and-expr1	AST_OR
*				/	\
*		and-expr2	and-expr3
*---------------------------------------------------------------------------===*/
ast *or_expr() {
	ast *t1, *t2, *t3;

	t1 = and_expr();
	if (tk == TK_OR) {
		tk = get_tk();
		t1 = t2 = new_node(AST_OR, t1, and_expr());
		check_and(t2);
		t2->ty = int_type;
		while (tk == TK_OR) {
			tk = get_tk();
			t3 = new_node(AST_OR, t2->right, and_expr());
			check_and(t3);
			t3->ty = int_type;
			t2->right = t3;
			t2 = t3;
		}
		t1 = fold_cnst_logic(t1);
	}
#if 0
	while (tk == TK_OR) {
		tk = get_tk();
		t2 = new_node(AST_OR, t1, NULL);
		t2->right = and_expr();
		check_and(t2);
		fold_cnst_logic(&t2, t2->left, t2->right, flag);
		t1 = t2;
		t2->ty = int_type;
		flag++;
	}
#endif

	return t1;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *fold_cond_expr(ast *t) {
	ast *cond, *t1 = t;

	cond = t->cond;
	if (t1->node == AST_COND && cond && cond->node == AST_CNST) {
		if (cond->u.cnst->u.cnst.i)
			t1 = t1->left;
		else
			t1 = t1->right;
	}

	return t1;
}

/*===-------------------------------------------------------------------------
*	cond-expr ==> { or-expr ? expr : } or-expr
*	or-expr1 ? expr1 : or-expr2 ? expr2 : or-expr3 -->
*			AST_COND		OR			or-expr
*			/	\
*		expr1	AST_COND
*				/	\
*			expr2	or-expr3
*---------------------------------------------------------------------------===*/
static ast *cond_expr() {
	ast *t1, *t2;

	t1 = or_expr();
	assert(t1->ty);
	if (tk == '?') {
		tk = get_tk();
		t2 = new_node(AST_COND, NULL, NULL);
		t2->cond = t1;
		t2->left = expression();
		expect_tk(':');
		/* condition-expression is special, 
		new node appear as old node's child not father*/
		t2->right = cond_expr();
		/* check semantics and evaluate the AST's type*/
		check_cond(t2);
		/* optimzation: if the conditional expression is constant value*/
		/* Attention: zcc can optimzate cond-expr, even though recursion
		because of cond-expr is right side conbination*/
		t1 = fold_cond_expr(t2);
		t1 = t2;
	}

	return t1;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
ast *cnst_expr() {
	ast *t = cond_expr();
	if (t->node != AST_CNST)
		error(-1, zcc_line_no, "need constant expression\n");

	return t;
}

/*===--------------------------------------------------------------------------
*	asgn-expr ==> { unary asgn-op } cond-expr
*	etc: unary1 = unary2 = cond-expr -->
*				AST_ASGN		OR		cond-expr
*				/	\
*			unary1	AST_ASGN
*					/	\
*				unary2	cond-expr
*---------------------------------------------------------------------------===*/
static ast *asgn_expr() {
	ast *t1, *t2;
	char asgn_set[] = {fsv_asgn, 0};
	/* will be back this point of lex-buffer*/
	char *pc_tmp = get_pc_terrible();
	/* pc always pointer to the current tk's next character*/
	int tk_tmp = tk;
	/* (expression) or (typename)*/
	if (tk == '(') {
		tk = get_tk();
		if (is_first(tk, first_decl)) {	/* (typename) cast-expression*/
			/* back to the original state, we known this is a condtional-expression*/
			set_pc_terrible(pc_tmp);
			tk = tk_tmp;
			return cond_expr();
		}
	}
	/* unary() also process the '('expression')'*/
	t1 = unary();
	if (is_first(tk, asgn_set)) {
		int node;
		switch (tk) {
		case '=':
			node = AST_ASGN;
			break;
		case TK_MUL_ASGN:
			node = AST_MUL_ASGN;
			break;
		case TK_DIV_ASGN:
			node = AST_DIV_ASGN;
			break;
		case TK_MOD_ASGN:
			node = AST_MOD_ASGN;
			break;
		case TK_ADD_ASGN:
			node = AST_ADD_ASGN;
			break;
		case TK_SUB_ASGN:
			node = AST_SUB_ASGN;
			break;
		case TK_LS_ASGN:
			node = AST_LS_ASGN;
			break;
		case TK_RS_ASGN:
			node = AST_RS_ASGN;
			break;
		case TK_BAND_ASGN:
			node = AST_BAND_ASGN;
			break;
		case TK_BXOR_ASGN:
			node = AST_BXOR_ASGN;
			break;
		case TK_BIOR_ASGN:
			node = AST_BIOR_ASGN;
			break;
		default:
			assert(0);
		}
		tk = get_tk();
		t2 = new_node(node, t1, NULL);
		t2->right = asgn_expr();
		check_asgn(t2);
		/* Attention: type of the AST maybe qualified because of type 
		shared with left operand, but it's should be ignored, 
		fortunately, this will not harm my modifiable lvalue check, 
		since zcc check the AST node type first*/
		t2->ty = t2->left->ty;
		return t2;
	} else {
		/* back to the original state, we known this is a condtional-expression*/
		set_pc_terrible(pc_tmp);
		tk = tk_tmp;
		return cond_expr();
	}

	assert(0);
}

/*===--------------------------------------------------------------------------
initizlizer:
	assignment-expression
	'{'initializer {,initializer} [,]'}'
--------------------------------------------------------------------------===*/
ast *initializer() {
	ast *t1, *t2;

	if (tk == '{') {
		tk = get_tk();
		t1 = t2 = new_node(AST_INIT_LIST, initializer(), NULL);
		t2->line_no = zcc_line_no;
		while (tk == ',') {
			if ((tk = get_tk()) == '}')
				break;
			t2->right = new_node(AST_INIT_LIST, initializer(), NULL);
			t2->right->line_no = zcc_line_no;
			t2 = t2->right;
		}
		expect_tk('}');		//eat the '}'
	} else {
		t1 = asgn_expr();
		t1->line_no = zcc_line_no;
	}

	return t1;
} 

/*===-------------------------------------------------------------------------
*	expression:
*		assignment-expression {, assignment-expression}
*	asgn-expr1, asgn-expr2 ==>
*			AST_COMMA
*			/	\
*	asgn-expr1	AST_COMMA
*					/	\
*			asgn-expr2	NULL
--------------------------------------------------------------------------===*/
ast *expression() {
	ast *t1, *t2, *t3;

	t3 = t1 = new_node(AST_COMMA, asgn_expr(), NULL);
	assert(t1->left->ty);
	/* all the ast struct oeject of function will not avaliable when 
	the function is compiled over*/
	t1->ty = t1->left->ty;
	while(tk == ',') {
		tk = get_tk();
		t2 = new_node(AST_COMMA, asgn_expr(), NULL);
		t2->ty = t2->left->ty;
		t1->right = t2;
		t1 = t2;
	}

	return t3;
}
