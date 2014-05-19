/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __TYPE_H__
#define __TYPE_H__

//struct member {
	//symbol *it;
	//struct member *next;
//};
/* type kind value*/
enum {
	CAT_NOP,
	/* void*/
	CAT_VOID,
	/* integer*/
	CAT_CHAR,
	CAT_UCHAR,
	CAT_SHORT,
	CAT_USHORT,
	CAT_INT,
	CAT_UINT,
	//CAT_ENUM,	// enumration
	/* float*/
	CAT_FLOAT,
	CAT_DOUBLE,
	/* derived type*/
	CAT_POINTER,	//cat_pointer must be here
	CAT_STRUCT,
	CAT_UNION,
	CAT_ARRAY,
	CAT_FUNCTION,
	/* virtual type category*/
	CAT_ARITH,
	CAT_SCALAR,
	CAT_SU,			/* struct or union*/
	CAT_VA,			/* variable arguments*/
	/* sentry*/
	CAT_LAST
};
/* type datastruct*/
typedef struct type_t type;
struct type_t {
	int cat;					/* type category*/
	type *bty;
	/* function symbol or struct symbol etc.. 
	Will be used when generate the ASM code*/
	struct symbol_t *sym;
	int size;
	/* if struct type, alignment is the size of first member, 
	when it is double, this is important*/
	int align;
	int is_const : 1;
	int is_volatile : 1;
	union {
		struct {
			struct slist_t *sl;	//struct member slist
		} s;
		struct {
			struct slist_t *ul;	//union member slist;
		} u;
		struct {
			struct slist_t *el;
		} e;
		struct {
			/* this is used in declaration and definition, the item type is struct type_t*/
			struct dlist_t *params_decl;
			int stack_size;
		} f;
		struct {
			int n;
		} a;
	} u;
};
/* global compiler internal type*/
extern type *char_type;
extern type *float_type;
extern type *double_type;
extern type *int_type;
extern type *long_type;
extern type *short_type;
extern type *long_double;
extern type *signed_char;
extern type *unsigned_char;
extern type *unsigned_int;
extern type *unsigned_long;
extern type *unsigned_short;
extern type *void_type;
extern type *pointer_type;
/* interface*/
type *new_type(int cat, type *bty, int size, int align, int area);
type *new_array(type *base, int length, int area);
int is_compatiable(type *ty1, type *ty2);
int is_cpt_no_qual(type *ty1, type *ty2);
void complete_type(type *dst, type *src, int area);
//type *clone_type(type *ty, int area);
//type *clone_type_deep(type *ty, int area);
//type *clone_type_skin(type *ty);
type *qualify(type *ty, int is_const, int is_volatile, int area);
void enter_scope();
void exit_scope();
void init_types();
type *pointer(type *ref, int area);

#endif