/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __INFER_H__
#define __INFER_H__

#include "stmt.h"
/* check type category*/
#define is_low_cat(cat)		(cat == CAT_CHAR || cat == CAT_SHORT)
#define is_int_cat(cat)		(cat >= CAT_CHAR && cat <= CAT_UINT)
#define is_float_cat(cat)	(cat == CAT_FLOAT)
#define is_double_cat(cat)	(cat == CAT_DOUBLE)
#define is_arith_cat(cat)	(cat >= CAT_CHAR && cat <= CAT_DOUBLE)
/* scalar is_arith_cat(cat) or pointer*/
#define is_scalar_cat(cat)	(cat >= CAT_CHAR && cat <= CAT_POINTER)
/* not drived type*/
#define is_base_cat(cat)	(is_arith_cat(cat) || cat == CAT_STRUCT || cat == CAT_UNION)
/*===--- interface---===*/
//void check_args(struct dlist_t *params, struct ast_t *t1);
void check_asgn(struct ast_t *tree);
void check_cond(struct ast_t *tree);
void check_and(struct ast_t *tree);
void check_bit_op(struct ast_t *tree);
void check_equ(struct ast_t *tree);
void check_rel(struct ast_t *tree);
void check_shift(struct ast_t *tree);
void check_add(struct ast_t *tree);
void check_mul(struct ast_t *tree);
void check_cast(struct ast_t *tree);
void check_unary(struct ast_t *tree);
void check_post(struct ast_t *tree);
struct type_t *raise_to_int(struct type_t *ty);
void need_type(struct ast_t *t, int cat);
void need_no_null(void *p, int line, char *s);
/* fold constant*/
void fold_unary_cnst(int op, struct ast_t *t);
//void fold_cnst_arith(struct ast_t **tt, struct ast_t *lt, struct ast_t *rt);
//void fold_cnst_shift(struct ast_t **tt, struct ast_t *lt, struct ast_t *rt);
//void fold_cnst_bitwise(struct ast_t **tt, struct ast_t *lt, struct ast_t *rt);
struct ast_t *fold_cnst_logic(struct ast_t *t);
struct ast_t *fold_cnst_bin(struct ast_t *t);
struct type_t *get_arith_type(struct type_t *lty, struct type_t *rty);
void eval_node_set(struct ast_t *t, int *set);
int is_in_set(int t, int *set);

#endif
