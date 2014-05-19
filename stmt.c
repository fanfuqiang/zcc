/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

/* keep the AST for a whole function*/
ast *root_ast;
/* iteration-statement head*/
static dlist *iter_stack;
/* iteration-statement tail*/
static dlist *iter_esp;
static ast *statement();
static ast *comp_stmt();

/*===----------------------------------------------------------------------------------

------------------------------------------------------------------------------------===*/
ast *new_node(int node, ast *left, ast *right) {
	/* allocation area must be at AST, no ast will generate for global variable*/
	ast *p;
	NEW0(p, AST);

	p->node = node;
	p->left = left;
	p->right = right;
	p->line_no = zcc_line_no;

	return p;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
static void pop_iter() {
	//dlist *p;
	if (iter_esp) {
		iter_esp = iter_esp->prev;
		if (iter_esp)
			iter_esp->next = NULL;
		else
			/* if pop the last node*/
			iter_stack = NULL;
	} else
		assert(0);
	
	return;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
static void push_iter(ast *it) {
	dlist *d1, **dd = &iter_stack;;

	NEW0(d1, FUNC);
	d1->it = it;
	iter_esp = d1;

	if (iter_stack == NULL)
		iter_stack = d1;
	else {
		/* find the tail*/
		while ((*dd)->next)
			dd = &(*dd)->next;
		/* (*dd) is the last node in this list*/
		(*dd)->next = d1;
		d1->prev = *dd;
	}

	return;
}

/*===------------------------------------------------------------------------------

-------------------------------------------------------------------------------===*/
ast *switch_stmt() {
	ast *t1, *expr;

	expect_tk(TK_SWITCH);
	t1 = new_node(AST_SWITCH, NULL, NULL);
	expect_tk('(');
	t1->expr[0] = expr = expression();
	if (! is_int_cat(expr->ty->cat))
		error(-1, t1->line_no, "the control expression of a switch statement shall have integral type");
	expect_tk(')');
	/* i can distinguish iteration-statement type by t1->node*/
	push_iter(t1);
	t1->right = statement();
	pop_iter();

	return t1;
}

/*===----------------------------------------------------------------------------------
if (expression) statement
if (expression) else statement
-----------------------------------------------------------------------------------===*/
ast *if_stmt() {
	ast *t1;

	expect_tk(TK_IF);
	t1 = new_node(AST_IF, NULL, NULL);
	expect_tk('(');
	t1->expr[0] = expression();
	expect_tk(')');
	/* only AST_IF store its statement in the right hand*/
	t1->left = statement();
	if (peek() == TK_ELSE) {
		expect_tk(TK_ELSE);
		//t2 = new_node(AST_ELSE, NULL, NULL);
		t1->right = statement();
	}

	return t1;
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
ast *label_stmt() {
	ast *t1;
	symbol *lab = look_up(labels_tb, lex_data.name);
	if (lab)
		error(-1, zcc_line_no, "duplacated label %s\n", lex_data.name);
	else
		lab	= install(&labels_tb, lex_data.name, LOCAL, FUNC);
	t1 = statement();

	//lab->u.l.tree = t1;
	/* label is referenced by more than */
	//add2dlist_head(&lab->u.l.refs_ast, t1, FUNC);
	/* AST has more than one label*/
	add2slist_head(&t1->labels, lab, FUNC);

	return t1;
}

/*===----------------------------------------------------------------------------------
for '(' {expression}; {expression}; {expression}')' statement
-----------------------------------------------------------------------------------===*/
ast *for_stmt() {
	ast *t1;

	expect_tk(TK_FOR);
	t1 = new_node(AST_FOR, NULL, NULL);
	expect_tk('(');
	t1->expr[0] = expression();
	expect_tk(';');
	t1->expr[1] = expression();
	expect_tk(';');
	t1->expr[2] = expression();
	expect_tk(')');
	push_iter(t1);
	t1->right = statement();
	pop_iter();

	return t1;
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
ast *do_stmt() {
	ast *t1;

	expect_tk(TK_DO);
	t1 = new_node(AST_DO, NULL, NULL);
	push_iter(t1);
	t1->right = statement();
	pop_iter();
	expect_tk('(');
	t1->expr[0] = expression();
	expect_tk(')');
	expect_tk(';');
	
	return t1;
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
ast *while_stmt() {
	ast *t1;

	expect_tk(TK_WHILE);
	t1 = new_node(AST_WHILE, NULL, NULL);
	expect_tk('(');
	t1->expr[0] = expression();
	expect_tk(')');
	push_iter(t1);
	t1->right = statement();
	pop_iter();

	return t1;
}

/*===----------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
ast *expr_stmt() {
	ast *t1 = NULL;

	/* empty statement*/
	if (tk == ';')
		return NULL;
	t1 = expression();
	expect_tk(';');

	return t1;
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
ast *goto_stmt() {
	ast *t1;
	symbol *s;
	//dlist *d1;

	expect_tk(TK_GOTO);
	t1 = new_node(AST_GOTO, NULL, NULL);
	expect_tk(TK_ID);
	s = look_up(labels_tb, lex_data.name);
	if (s == NULL)
		s = install(&labels_tb, lex_data.name, LOCAL, FUNC);
	/* attention this type is symbol, because we have no AST_LABEL node*/
	t1->dst = s;
#if 0
	/* TODO: this useless need be clean, will be done in icode pass perfectly*/
	d1 = s->u.l.refs_ast;
	while (d1 && d1->it != (void *)t1)
		d1 = d1->next;
	if (d1 == NULL)
		/* label may be referenced by more than one AST*/
		add2dlist_head(&s->u.l.refs_ast, t1, FUNC);
#endif
	expect_tk(';');

	return t1;
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
ast *return_stmt() {
	ast *t1;

	expect_tk(TK_RETURN);
	t1 = new_node(AST_RETURN, NULL, NULL);
	if (tk != ';')
		t1->right = expression();
	expect_tk(';');
	
	return t1;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
ast *break_stmt() {
	ast *t1;

	expect_tk(TK_BREAK);
	t1 = new_node(AST_BREAK, NULL, NULL);
	if (iter_esp == NULL)
		error(-1, t1->line_no, "break has no enclosing switch-statement\n");
	else
		t1->dst = iter_esp;
	expect_tk(';');

	return t1;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
ast *continue_stmt() {
	ast *t1, *t2;
	dlist *p = iter_esp;

	expect_tk(TK_CONTINUE);
	t1 = new_node(AST_CONTINUE, NULL, NULL);
	/* continue should inside one loop-statement*/
	while (p && ((ast *)(p->it))->node == AST_SWITCH)
		p = p->prev;
	if (p == NULL)
		error(-1, t1->line_no, "continue has no enclosing iteration-statement\n");
	else {
		t2 = (ast *)(p->it);
		assert(t2->node == AST_FOR || t2->node == AST_WHILE 
			|| t2->node == AST_DO);
		t1->dst = t2;
	}
	expect_tk(';');

	return t1;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
ast *find_switch() {
	//ast *t1;
	dlist *p = iter_esp;

	while (p && ((ast *)p->it)->node != AST_SWITCH)
		p = p->prev;
	if (p == NULL)
		return NULL;
	else
		return (ast *)p->it;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
static void add2switch(ast *sw, ast *ca) {
	dlist *dc = sw->cases;
	int n;
	ast *t1;

	assert(ca->node == AST_CASE || ca->node == AST_DEFAULT);
	/*==-- default-statement--==*/
	if (ca->node == AST_DEFAULT) {
		if (sw->def)
			error(-1, sw->line_no, "there may be at most one default label in this switch statement\n");
		//add2slist_head(&sw->def, ca, FUNC);
		sw->def = ca;
		return;
	}
	/*==-- case-statement--==*/
	n = ca->u.cnst->u.cnst.i;
	/* switch statement already has case-statement*/
	if (dc) {
		//dl = sw->u.cases;
		t1 = (ast *)dc->it;
		while (dc && t1->u.cnst->u.cnst.i < n) {
			//dl2 = dl;
			dc = dc->next;
			if (dc)
				t1 = (ast *)dc->it;
		}
		/* ca's value is biggest*/
		if (dc == NULL)
			add2dlist_tail(&sw->cases, ca, FUNC);
		else if (t1->u.cnst->u.cnst.i == n)
			error(-1, t1->line_no, "two case-statement should not hava same value in same switch-statement\n");
		/* t1 takes the big one*/
		else {
			dlist *prev;
			prev = dc->prev;
			/* if dc not the first case-statement in cases*/
			if (prev)
				insert2dlist(&prev, ca, FUNC);
			else
				add2dlist_head(&sw->cases, ca, FUNC);
		}
	} else
		add2dlist_head(&sw->cases, ca, FUNC);

	return;
}

/*===---------------------------------------------------------------------------------
case constant-expression : statement
----------------------------------------------------------------------------------===*/
ast *case_stmt() {
	ast *t1, *t2, *cnst;

	expect_tk(TK_CASE);
	t1 = new_node(AST_CASE, NULL, NULL);
	expect_first_cur(first_cnst);
	cnst = cnst_expr();
	need_type(cnst, CAT_INT);
	t1->u.cnst = cnst->u.cnst;
	expect_tk(':');
	if ((t2 = find_switch()) == NULL)
		error(-1, t2->line_no, "case-statement has no enclosing switch-statement\n");
	else
		t1->dst = t2;
	t1->right = statement();
	/* */
	add2switch(t2, t1);
	//add2slist_head(&t2->u.cases, t1, FUNC);

	return t1;
}

/*===---------------------------------------------------------------------------------
default : statement
----------------------------------------------------------------------------------===*/
ast *default_stmt() {
	ast *t1, *t2;

	expect_tk(TK_DEFAULT);
	t1 = new_node(AST_DEFAULT, NULL, NULL);
	expect_tk(':');
	if ((t2 = find_switch()) == NULL)
		error(-1, t1->line_no, "default-statement has no enclosing switch-statement\n");
	t1->dst = t2;
	t1->right = statement();
	/* when generate the icode for the switch statement, 
	we must check that there may be at most one TK_DEFAULT label in this switch statement*/
	//add2slist_head(&t2->u.def, t1, FUNC);
	add2switch(t2, t1);

	return t1;
}

/*===-----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
static ast *statement() {
	ast *t1;
	int t;

	switch (tk) {
		case TK_ID:
			t = peek();
			if (t == ':')
				t1 = label_stmt();
			else
				t1 = expr_stmt();
			break;
		case TK_CASE:
			t1 = case_stmt();
			break;
		case TK_DEFAULT:
			t1 = default_stmt();
			break;
		case '{':
			t1 = comp_stmt();
			break;
		case TK_IF:
			t1 = if_stmt();
			break;
		case TK_ELSE:
			assert(0);
		case TK_SWITCH:
			t1 = switch_stmt();
			break;
		case TK_WHILE:
			t1 = while_stmt();
			break;
		case TK_DO:
			t1 = do_stmt();
			break;
		case TK_FOR:
			t1 = for_stmt();
			break;
		case TK_GOTO:
			t1 = goto_stmt();
			break;
		case TK_CONTINUE:
			t1 = continue_stmt();
			break;
		case TK_BREAK:
			t1 = break_stmt();
			break;
		case TK_RETURN:
			t1 = return_stmt();
			break;
		default:
			if (tk != ';')
				expect_first_cur(first_cnst);
			t1 = expr_stmt();
	}

	return t1;
}

/*===----------------------------------------------------------------------------------
compound-statement
	'{' {declaration-list} {statement-list}'}'
-----------------------------------------------------------------------------------===*/
static ast *comp_stmt() {
	ast *t1, *t2, *t3;

	enter_scope();
	t1 = new_node(AST_BLOCK, NULL, NULL);
	/* {declaration-list}*/
	while (is_first(tk, first_decl))
		decl_local();
	/* {statement-list}*/
	t3 = t1;
	while (tk != '}') {
		t2 = new_node(AST_LIST, NULL, NULL);
		t2->left = statement();
		t3->right = t2;
		t3 = t2;
	}
	expect_tk('}');
	exit_scope();
	
	return t1;	/* return a local variable, but this is ok*/
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
static void reset_func_data() {
	/* clean this*/
	memset(&_F, 0, sizeof(_F));

	return;
}

/*===-----------------------------------------------------------------------------------
'{' [declarations] statement-list'}'
this function should eat the '{' and '}' pair
-----------------------------------------------------------------------------------===*/
void func_define() {
	dlist *p;

	assert(tk == '{');
	if (level != GLOBAL)
		error(-1, zcc_line_no, "function define should be in external\n");
	_F.cur_func->is_defined = 1;
	root_ast = new_node(AST_FUNC, NULL, NULL);
	//root_ast->node = AST_FUNC;
	//root_ast->dst = cur_func;
	assert(_F.cur_func->ty);
	/* install the parameters*/
	for (p = _F.cur_func->ty->u.f.params_decl; p && p != _F.cur_func->ty->u.f.params_decl; p = p->next) {
		symbol *s = (symbol *)p->it;
		/* may be refrenced by other, so need PERM*/
		if (s->ty->cat != CAT_VOID && s->name == NULL)
			error(-1, zcc_line_no, "parameter has no name\n");
		install(&ids_tb, s->name, LOCAL, PERM);
	}
	tk = get_tk();
	/* set current block, because comp_stmt will call enter_scope*/
	assert(level == GLOBAL);
	//level = LOCAL - 1;
	root_ast->right = comp_stmt();
	ast2icode(root_ast);
	opt_ic(_F.root_ic);
	gas_glue(_F.root_ic);
	/* reset data for next function assembler file generate*/
	reset_func_data();

	return;
}
