/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __DECL_H__
#define __DECL_H__

/* first set value*/
enum first_set_value {
	fsv_nop,
	fsv_base,		/* base type*/
	fsv_qual,		/* qualifiers*/
	fsv_scls,		/* storeage class*/
	fsv_atom,		// cnst
	fsv_id,			// identifier
	fsv_asgn,
	fsv_pre_op,		// ++, --, sizeof
	fsv_unary_op,	// &, *, +, -, ~, !
	fsv_rel_op,		// <, >, <=, >=
	fsv_lp,			// '('
	fsv_lab,		// case, default
	fsv_sel,		// if, switch
	fsv_iter,		// while, do, for
	fsv_jmp,		// goto, continue, break, return
	fsv_last
};
/* stack offset of the current function*/
#define cur_stk_os _F.cur_func->ty->u.f.stack_size
/* global data for function code generation*/
struct func_data_t {
	struct symbol_t *cur_func;
	struct symbol_t *cur_func_end;
	/* symbol slist only uesd conveniently when generate ASM file*/
	struct slist_t *data_ids;
	struct slist_t *rodata_strings;			//strings need output to ASM file
	struct slist_t *rodata_cnsts;
	//struct slist_t *data_inits;
	struct slist_t *stack_inits;
	//struct icode_t *ic_inits;
	struct icode_t *root_ic;
	struct icode_t *tail_ic;
	int cur_ebp_offset;
	int cur_args_size;
	struct symbol_t spill_i32;
	struct symbol_t spill_f32;
	struct symbol_t spill_f64;
};
extern struct func_data_t _F;

/*global variable for current level*/
extern int level;
//#define is_first_spec(t) (t > 0 && t < TK_ID || t == TK_TYPE)
/* assert function*/
#ifdef ZCC_ASSERT
#define zcc_assert(expr) assert(expr)
#else
#define zcc_assert(expr)
#endif

/* lex data*/
union u_lex_internal_t {
	char *name;
	struct symbol_t *sym;
	struct type_t *ty;
};
/* first sets*/
extern char first_cnst[];
extern char first_decl[];
/* data*/
extern union u_lex_internal_t lex_data;
extern char *g_tk2str[];
/* interface*/
void main_42();
void init_spill_loc();
//void init_tk2str();
void decl_local();
void expect_tk(int t);
void expect_tk_cur(int t);
void expect(char set[]);
void expect_cur(char set[]);
void expect_first(char set[]);
void expect_first_cur(char set[]);
type *specifier(int *sclass);
int is_first(int t, char expect_set[]);

#endif