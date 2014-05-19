/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __ICODE_H__
#define __ICODE_H__

/* postfix value*/
enum ic_postfix {
	i0,		/* constant ?*/
	i8,		/* char*/
	i16,
	i32,	/* int*/
	//p32,
	f32,	/* float*/
	f64,	/* double*/
	/* type convert*/
	i8_i32,		//convert from i32 to i8
	i8_f32,
	i8_f64,
	i32_i8,
	//i32_u8,
	//i32_p32,
	i32_f32,
	i32_f64,
	f32_i8,
	f32_i32,
	f32_f64,
	f64_i8,
	f64_i32,
	f64_f32,
	/* compiler internal type convert*/
	//i8_f80,
	//i32_f80,
	//f32_f80,
	//f64_f80,
	//f80_i32,
	//f80_f32,
	//f80_f64
};

/* icode value enumrator*/
enum ic_pattern {
#define inst(a, b, c, d) a = b << 5,
#define inst_last(a, b, c, d) a = b << 5
//#define inst(a, b, c, d) a,
//#define inst_last(a, b, c, d) a
#include "inst.h"
#undef inst_last
#undef inst
};
/* icode*/
typedef struct icode_t icode;
struct icode_t {
	int node;					/* icode type*/
	//char *inst;					/* instruction*/
	struct symbol_t *left;		/* also is the result*/
	struct symbol_t *right;
	icode *prev;
	icode *next;
	//struct slist_t *label;
	union {
		int arg_off;
	} u;
};
/* data*/
extern struct ast_t *ast_cnst_zero;
extern struct ast_t *ast_cnst_one;
extern struct symbol_t *cnst_one;
extern struct symbol_t *cnst_zero;
/* interface*/
void ast2icode(struct ast_t *tree);
void init_cnst_ast();
//struct symbol_t *asgn2ic(struct ast_t *t1);
//struct symbol_t *cast(struct symbol_t *from, struct type_t *to);
int gen_post(int tc, int fc);
//struct icode_t *emit_ic(int node, struct symbol_t *l, struct symbol_t *r);

#endif
