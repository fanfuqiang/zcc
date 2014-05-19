/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __STMT_H__
#define __STMT_H__

enum {
	/*==-- trival--==*/
	AST_NOP,
	/*==-- statement--==*/
	AST_FUNC,
	AST_BLOCK,			/* compound statement*/
	AST_IF,
	AST_DO,
	AST_FOR,
	AST_WHILE,
	AST_SWITCH,
	AST_GOTO,
	AST_BREAK,
	AST_CONTINUE,
	AST_RETURN,			/* return statement*/
	AST_CASE,
	AST_DEFAULT,
	AST_LIST,			/* statement list*/

	/*==-- expression--==*/
	AST_COMMA,			/* comma expression*/
	AST_ARGS,
	/* assignment-expression*/
	AST_ASGN,			/* = operator*/
	AST_MUL_ASGN,		/* *= operator*/
	AST_DIV_ASGN,		/* /= operator*/
	AST_MOD_ASGN,		/* %= operator*/
	AST_ADD_ASGN,		/* += operator*/
	AST_SUB_ASGN,		/* -= operator*/
	AST_LS_ASGN,		/* <<= operator*/
	AST_RS_ASGN,		/* >>= operator*/
	AST_BAND_ASGN,		/* &= operator*/
	AST_BXOR_ASGN,		/* ^= operator*/
	AST_BIOR_ASGN,		/* |= operator*/
	AST_COND,			/* or-expr ? expr: cond-expr*/
	/* binary operator, the order is sensitive*/
	AST_OR,				/* ||*/
	AST_AND,			/* &&*/
	AST_BIOR,			/* |*/
	AST_BXOR,			/* ^*/
	AST_BAND,			/* &*/
	AST_EQU,			/* ==*/
	AST_UE,				/* !=*/
	AST_G,				/* >*/
	AST_L,				/* <*/
	AST_GE,				/* >=*/
	AST_LE,				/* <=*/
	AST_LS,				/* <<*/
	AST_RS,				/* >>*/
	AST_ADD,			/* +*/
	AST_SUB,			/* -*/
	AST_MUL,			/* **/
	AST_DIV,			/* /*/
	AST_MOD,			/* %*/
	/* unary-expression*/
	AST_CAST,			/* type-cast*/
	AST_ADDR,			/* & operator*/
	AST_IND,			/* * operator*/
	AST_PST,			/* + operator*/
	AST_NEG,			/* - operator*/
	AST_COM,			/* ~ operator*/
	AST_NOT,			/* ! operator*/  
	AST_PRE_INC,		/* ++ unary*/
	AST_PRE_DEC,		/* -- unary*/
	//AST_SIZEOF,		/* sizeof expr*/
	/* postfix-expression*/
	AST_POST_INC,		/* unary ++*/
	AST_POST_DEC,		/* unary --*/
	AST_ARRAY,			/* [] operator*/
	AST_CALL,			
	AST_MEMBER,			/* . operator*/
	AST_ARROW,			/* -> operator*/
	/* primary-expression*/
	AST_ID,
	AST_CNST,
	AST_LIT,			/* string literal*/
	AST_PACK_EXPR,		/* (expression)*/
	AST_INIT_LIST,		/* initializer-list*/
	/*==-- sentry item--==*/
	AST_LAST
};

/* define the struct of AST*/
typedef struct ast_t ast;
struct ast_t {
	int node;						/* node kind, it's value defined in above anonymous enum*/
	int line_no;
	struct type_t *ty;				/* result type of this AST*/
	struct ast_t *left;				/* left child*/
	struct ast_t *right;			/* right child*/
	struct ast_t *expr[3];			/* why is 3? because the for statemant*/
	struct ast_t *cond;				/* cond ? expr : expr*/
	struct slist_t *labels;			/* labels_tb for the ast*/
	struct dlist_t *cases;			/* used by switch statement, attention this is NO-circle doubly list*/
	struct ast_t *def;				/* default-statement*/
	void *dst;						/* this AST's enclosing iteration-statement, or some other purposes*/
	struct symbol_t *ct_label;		/* continue label for loop statement*/
	struct symbol_t *bk_label;		/* break label for loop and switch statement*/
	/* primary*/
	union {
		struct symbol_t *cnst;		/* constant expression*/
		struct symbol_t *id;		/* primary-expression : identifier*/
		//char *lit;				/* literal constant*/
		int arg_off;				// only for AST_ARGS node
	} u;
};

//extern struct dlist_t *iter_esp;

void func_define();
ast *new_node(int node, ast *left, ast *right);


#endif