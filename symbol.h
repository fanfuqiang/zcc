/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

/* express scope*/
enum level_int {
	MEMBER = -1,
	l_undef = 0,
	CONSTANT = 1,
	GLOBAL,
	LOCAL
};
/* constant value*/
union value_t {
	char c;
	int i;
	void *p;
	float f;
	double d;
	char a[8];		/* for convenient reference of struct union cnst*/
};
typedef union value_t value;
/* symbol data*/
typedef struct symbol_t symbol;
struct symbol_t {
	char *name;
	char *asm_name;			/* static identifier*/
	int scope;
	/* storeage class*/
	struct type_t *ty;
	/* linkage can only has one, so when define the linkage must make sure this*/
	int is_internal : 1;	/* is internal linkage*/
	int is_external : 1;	/* is external linkage*/
	//int is_global : 1;	/* need this??*/
	int is_no_linkage : 1;	/* is no linkage*/
	int is_static : 1;		/* only used by block scope static identifier, which has no linkage*/
	int is_typedef : 1;		/* is typedef defined type name*/
	int is_register : 1;
	/* qualifier*/
	int is_const : 1;
	int is_volatile : 1;
	int is_defined : 1;
	int is_bitfield : 1;
	//int is_agg_pad : 1;
	//int is_struct : 1;
	//int is_union : 1;
	int is_enum : 1;
	//int is_extern : 1;		/* the storeage of this identifier is extern*/
	int is_used : 1;		/* extern identifier has been used*/
	int is_immediate : 1;	// constants
	/*==-- backend data--==*/
	//int is_register : 1;
	int is_itmp : 1;		/* virtual register*/
	int is_split : 1;		/* s->reg is a spill location not a register*/
	int is_ignore_alter : 1;	//alter()
	int is_need_wb : 1;		/* need write back ?*/
	int is_in_reg : 1;		/* lvalue is in register*/
	int is_i8 : 1;
	int is_addr : 1;		/* indirect access, ASM output like this: offset(register-name)*/
	int is_need_st : 1;		/* set if need fpu register st(x)*/
	int is_emitted : 1;		/* has been printed in asm file*/
	int is_free : 1;		/* if this symbol is a register*/
	union {
		union value_t cnst;
		struct {
			short offset;		/* offset against the struct, object size can be 32767 =  2^15*/
			char bits_len;
			char bits_pos;
		} s;	///struct member
		struct {
			//struct dlist_t *refs_ast;		/* maybe the target of more than one instruction*/
			struct dlist_t *refs_ic;
		} l;	///label
		struct {
			/* only uesd when function is defined, the item type is symbol*/
			struct dlist_t *parmas_def;
			int stack_size;
			/* the size of current calling's arguments*/
			//int cur_call_size;
		} f;
		struct {
			struct dlist_t *alias;
		} reg;
	} u;	/* i32 x 2*/
	/*===--- backend data area---===*/
	/* operand kind, when some local type data is freed, also express register kind*/
	char *name_i8_low;				// for eax this is al
	//char *name_r8_high;
	//int op_kind;
	int use_times;
	//struct icode_t *ind_ic;	/* the symbol belong to which icode, specifial for AST_IND node*/
	int ebp_off;				/* also as allocated spill locatation*/
	//int index;					/* related with base name, for example : member*/
	symbol *reg;
	struct ast_t *iz;
};
/* symbol table*/
typedef struct entry_t {
		symbol sym;
		struct entry_t *next;
} entry; 
typedef struct sym_tab_t sym_tab;
struct sym_tab_t {
	int scope;
	sym_tab *prev;
	entry *buckets[H_SIZE + 1];
};
/* data*/
extern sym_tab *globals_tb;
extern sym_tab *ids_tb;
extern sym_tab *cnsts_tb;
//extern sym_tab *types_tb;
extern sym_tab *tags_tb;
extern sym_tab *labels_tb;
/* interface*/
//void init_symtab();
symbol *install(sym_tab **st, char* name, int level, int area);
char *gen_tmp_name(char *s);
symbol *gen_label(char *pat);
//symbol *gen_tmp(sym_tab **, char *, int);
symbol *constant(value v, struct type_t *ty);
symbol *look_up(sym_tab *tab, char *name);
symbol *look_up_level(sym_tab *st, char *name, int lev);
symbol *lookup2(sym_tab **, char *, int);
symbol *gen_symbol(char *name, struct type_t *ty, int area);

#endif