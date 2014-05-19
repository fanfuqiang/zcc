/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

static type *dclr(type *bty, char **name, type **patch_addr);
/* global data for function code generation*/
struct func_data_t _F;
/* will be access everywhere, represent block level*/
int level = GLOBAL;
/* global data, will be write by lex*/
union u_lex_internal_t lex_data;
/* FIRST set, help implementation the LL(1) algorithm*/
char map_first[] = {
#define native(a, b, c, d) c,
#define token(a, b, c, d) c,
#include "token.h"
#undef native
#undef token
};
/* for print accurate error message*/
char *g_tk2str[] = {
#define native(a, b, c, d) d,
#define token(a, b, c, d) d,
#include "token.h"
#undef native
#undef token
};
/* first set of constant-expression*/
char first_cnst[] = {fsv_lp, fsv_unary_op, fsv_pre_op, fsv_id, fsv_atom, 0};
char first_decl[] = {fsv_base, fsv_qual, fsv_scls, 0};
/* follow set*/
static char follow_tk_decl[] = {'=', '{', ';', ',', 0};

/*===----------------------------------------------------------------------------------
the last element of expect_set[] is 0
-----------------------------------------------------------------------------------===*/
int is_first(int t, char expect_set[]) {
	char *p = expect_set;

	assert(*p);
	for (; *p && map_first[t] != *p; p++)
		;	// nothing
	if (*p == 0)
		return 0;
	else {
		assert(map_first[t] == *p);
		return 1;
	}
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
static char *fill_expect_token(char set[]) {
	char *p = set;
	char *buffer = buffer_256;
	int size = 0;

	for (; *p; p++, buffer += size) {
		size = sprintf(buffer, "%s ", g_tk2str[*p]);
		size = sprintf(buffer + size, " or ");
		assert(buffer + size < buffer_256 + 256);
	}
	
	return buffer_256;
}

/*===----------------------------------------------------------------------------------

-----------------------------------------------------------------------------------===*/
static char *fill_expect_first(char first_set[]) {
	char *p = first_set;
	char *buffer = buffer_256;
	int size = 0;

	for (; *p; p++, buffer += size) {
		switch (*p) {
		case fsv_base:
			size = sprintf(buffer, "%s ", "type-specifier");
			break;
		case fsv_qual:
			size = sprintf(buffer, "%s ", "type-qualifier");
			break;
		case fsv_scls:
			size = sprintf(buffer, "%s ", "storeage-class");
			break;
		case fsv_atom:
			size = sprintf(buffer, "%s ", "identifier constant string");
			break;
		case fsv_pre_op:
			size = sprintf(buffer, "%s ", "++, --, sizeof");
			break;
		case fsv_unary_op:
			size = sprintf(buffer, "%s ", "unary-operator");
			break;
		case fsv_rel_op:
			size = sprintf(buffer, "%s ", "relative-operator");
			break;
		case fsv_lp:
			size = sprintf(buffer, "%s ", "(");
			break;
		default :
			assert(!"first set error");
		}
		size = sprintf(buffer + size, " or ");
		assert(buffer + size < buffer_256 + 256);
	}

	return buffer_256;
}

/*===----------------------------------------------------------------------------------
set[] is a first value set
-----------------------------------------------------------------------------------===*/
void expect_first_cur(char set[]) {
	char *p = set;
	for (; *p == 0 || map_first[tk] == *p; p++)
		if (*p == 0)
			error(-1, zcc_line_no, "syntax illegal, expect: %s\n", fill_expect_first(set));
		else
			assert(map_first[tk] == *p);

	return;
}

/*===----------------------------------------------------------------------------------
set[] is a fisrt value set
----------------------------------------------------------------------------------===*/
void expect_first(char set[]) {
	char *p = set;
	for (; *p == 0 || map_first[tk] == *p; p++)
		if (*p == 0)
			error(-1, zcc_line_no, "syntax illegal, expect: %s\n", fill_expect_first(set));
		else
			assert(map_first[tk] == *p);
	tk = get_tk();

	return;
}

/*===----------------------------------------------------------------------------------
set[] is a token set
-----------------------------------------------------------------------------------===*/
void expect_cur(char set[]) {
	char *p = set;
	for (; *p == 0 || tk == *p; p++)
		if (*p == 0)
			error(-1, zcc_line_no, "syntax illegal, expect: %s\n", fill_expect_token(set));
		else
			assert(tk == *p);

	return;
}

/*===----------------------------------------------------------------------------------
set[] is a token set
----------------------------------------------------------------------------------===*/
void expect(char set[]) {
	char *p = set;
	for (; *p == 0 && tk == *p; p++)
		if (*p == 0)
			error(-1, zcc_line_no, "syntax illegal, expect: %s\n", fill_expect_token(set));
		else
			assert(tk == *p);
	tk = get_tk();

	return;
}

/*===----------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
void expect_tk_cur(int token) {
	if (tk != token)
		error(-1, zcc_line_no, "syntax illegal, expect: %s\n", g_tk2str[token]);

	return;
}

/*===----------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
void expect_tk(int token) {
	if (tk != token)
		error(-1, zcc_line_no, "syntax illegal, expect: %s\n", g_tk2str[token]);
	tk = get_tk();

	return;
}

/*===----------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
void alloc_stack(symbol *s) {
	int size = s->ty->size;
	
	assert(size);
	// ebp_off == 1, means this symbol is a typedef name or local extern object
	assert((s->ebp_off == 0 || s->ebp_off == 1) && "Panic, alloc internal stack error");
	if (s->ebp_off)		//offset == 1
		return;
	_align(&_F.cur_ebp_offset, size);
	s->ebp_off = _F.cur_ebp_offset;
	_F.cur_ebp_offset += size;

	return;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
static void _bits_pos(type *pre_type, int *cur_pos, int *offset, symbol *s) {		
	int pre_size = pre_type->size;
	int size = s->ty->size;
	char *pos = &s->u.s.bits_pos;
	int len = s->u.s.bits_len;
	short *os = &s->u.s.offset;
	/* char : 3;  <-- pre, 
	   int : 5
	if this, updata offset and current position value*/
	if (pre_size != size) {
		*offset += pre_size;
		*cur_pos = 0;
	}
	/*attention this*/
	_align(offset, size);
	if (len == 0) {
		// int : 0;
		*os = (short)*offset;
		*cur_pos = 0;
		*offset += size;
		return;
	}
	/* the other 3 conditions*/
	if (*cur_pos + len == size) {
		*os = (short)*offset;
		*pos = (char)*cur_pos;
		*cur_pos = 0;
		*offset += size;
	} else if (*cur_pos + len > size) {
		*offset += size;
		/* attention this*/
		_align(offset, size);
		*os = (short)*offset;
		*pos = *cur_pos = 0;
		*cur_pos = len;
	} else {
		*os = (short)*offset;
		*pos = *cur_pos;
		*cur_pos += len;
	}

	return;
}

/*===---------------------------------------------------------------------------------

---------------------------------------------------------------------------------===*/
static void m_offset(symbol *sym) {
	int os = 0;			// offset;
	int cp = 0;			// current position
	type *pre_type = NULL;
	symbol *s = NULL;
	slist *lst = sym->ty->u.s.sl;

	if (sym->ty->cat == TK_UNION)
		return;
	assert(sym->ty->cat == TK_STRUCT);
	assert(lst);
	for (; lst; lst = lst->next) {
		s = (symbol *)lst->it;
		if (s->is_bitfield == 0) {
			_align(&os, s->ty->size);
			s->u.s.offset = os;
			os += s->ty->size;
		} else
			_bits_pos(pre_type, &cp, &os, s);
		/* updata*/
		pre_type = s->ty;
	}
	/*pad the struct*/
	_align(&os, sym->ty->align);
	sym->ty->size = os;

	return;
}

/*===---------------------------------------------------------------------------------

---------------------------------------------------------------------------------===*/
static void add_member(symbol *s, symbol *it) {
	slist **pp;

	if (s->ty->cat == CAT_STRUCT)
		pp = &s->ty->u.s.sl;
	else
		pp = &s->ty->u.u.ul;
	for (; *pp; pp = &(*pp)->next)
		if (((symbol *)(*pp)->it)->name = it->name)
			error(-1, zcc_line_no, "member of %s has same name\n", s->name);
	NEW0(*pp, level > GLOBAL ? TY : PERM);
	(*pp)->it = it;

	return;
}

/*===----------------------------------------------------------------------------------
m_define -- member definition
	struct-declarator-slist
		{struct-declarator}+
	struct-declarator
		[declarator] [: constant-expression]
-----------------------------------------------------------------------------------===*/
static void m_define(symbol *s) {
	type *spec = NULL, *ty;
	int sclass = 0;
	symbol *ds = NULL;	//declarator's return symbol
	char *name = NULL;
	ast *tree;
	int len;
	int max_size = 0;
	/* members*/
	while (1) {
		char set[] = {fsv_base, fsv_qual, 0};
		//if (!is_first(tk, set))
			//error(-1, zcc_line_no, "expect type specifier or qualifier\n");
		expect_first_cur(set);
		spec = specifier(&sclass);
		assert(spec);
		/* declarators*/
		do {
			NEW0(ds, level > GLOBAL ? FUNC : PERM);
			/*no declarator, such as --> int : 5*/
			if (tk == ':') {
				char set[] = {fsv_atom, fsv_lp, fsv_pre_op, fsv_unary_op, 0};
				name = NULL;
				ty = spec;
				expect_first_cur(set);
				/* do not eat ':'*/
			} else
				/*dclr() should not add new symbol into any sym_tab*/
				ty = dclr(spec, &name, NULL);
			/* bit-field*/
			if (tk == ':') {
				tk = get_tk();
				expect_first_cur(first_cnst);
				tree = cnst_expr();
				if (ty->size == 0)
					error(-1, zcc_line_no, "member can not be incomplete type\n");
				if (tree->node != AST_CNST || !is_int_cat(tree->u.cnst->ty->cat))
					error(-1, zcc_line_no, "bit-field length is illegal\n");
				if ((len = tree->u.cnst->u.cnst.i) > ty->size)
					error(-1, zcc_line_no, "bit-field length is too large\n");
				ds->u.s.bits_len = len;
				ds->is_bitfield = 1;
			} else {
				/* Attention: do not break the outer set[]*/
				//char set[] = {',', 0};
				expect_tk_cur(',');
			}
			/* add member in struct's list*/
			ds->name = name;
			ds->ty = ty;
			if (ty->size > max_size)
				max_size = ty->size;
			add_member(s, ds);
			/* this declaration line is over*/
			if (tk == ';') {
				tk = get_tk();
				break;
			}
			
		} while (tk == ',');
		/* member definition is over*/
		if (tk == '}')
			break;
	}
	/* struct/union alignment >= 4, is it correct?*/
	max_size < 4 ? max_size = 4 : 42;
	if (s->ty->cat == TK_UNION)
		s->ty->size = s->ty->align = max_size;
	else {
		s->ty->align = max_size;
		m_offset(s);
	}
	/* current token is '}'*/
	return;
}

/*===----------------------------------------------------------------------------------
deal with the definition and declaration of struct and union 
----------------------------------------------------------------------------------===*/
static type *struct_spec() {
	symbol *s = NULL;
	char *name = NULL;
	//char set[] = {0, 0};
	int cat;
	int define = 0;
	/*==---------------------------------------------------------------------
	1, struct tag {member_define}
	2, struct {member_define}
	3, struct tag
	4, struct tag; <-- token ';' has beyond the token flow of struct_spec()
	----------------------------------------------------------------------==*/
	assert(tk == TK_STRUCT || tk == TK_UNION);
	if (tk == TK_STRUCT)
		cat = CAT_STRUCT;
	else
		cat = CAT_UNION;
	/// eat the struct or union
	tk = get_tk();
	if (tk == TK_ID) {
		tk = get_tk();
		name = lex_data.name;
	} else {
		//set[0] = '{';
		expect_tk_cur('{');
		if (cat == CAT_STRUCT)
			name = gen_tmp_name("struct");
		else
			name = gen_tmp_name("union");
	}
	/* 1, struct { members } 
	2, struct tag { members } */
	if (tk == '{') {
		/// we can sure this is a struct type definition
		//set[0] = '}';
		s = look_up_level(tags_tb, name, level);
		if (s && s->is_defined)
			error(-1, zcc_line_no, "struct %s have be defined in same scope\n", name);
		define = 1;
	} else {
		/* 3, struct tag <-- the current token is not ';'*/
		if (tk != ';' && (s = look_up(tags_tb, name)) && s->ty->cat == cat)
			return s->ty;
	}
	/* 1/2, struct [tag] '{' members '}' <--creat new struct type*/
	/* 4, struct tag; current token is ';' <-- need creat new incomplete struct type*/
	s = install(&tags_tb, name, level, 0);
	/* declaration more than one time in the same scope is legal*/
	if (s->ty == NULL) {
		s->ty = new_type(cat, NULL, 0, 0, 0);
		s->ty->sym = s;
	}
	if (define) {
		/* define members, current token is '{'*/
		tk = get_tk();
		m_define(s);
		expect_tk('}');
		s->is_defined = 1;
	}

	return s->ty;
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
static void e_define() {
	char *name;
	symbol *sym;
	int prev = 0;
	ast *val;

	while (1) {
		//char set[] = {TK_ID, 0};
		expect_tk(TK_ID);
		name = lex_data.name;
		/*enumrator is very special, looks like has a TK_TYPE, 
		but just some symbol has constant value. i give enumrator the int type*/
		if (sym = look_up_level(ids_tb, name, level))
			error(-1, zcc_line_no, "redifiniton of name: %s", name);
		else {
			sym = install(&ids_tb, name, level, 0);
			/* constant value*/
			sym->is_immediate = 1;
			sym->ty = int_type;
		}
		/*Attention : important, maybe need modify*/
		if (tk == '=') {
			expect_first_cur(first_cnst);
			val = cnst_expr();
			if (val->node == AST_CNST && is_int_cat(val->ty->cat))
				sym->u.cnst.i = val->u.cnst->u.cnst.i;
			else
				error(-1, zcc_line_no, "enumation constant expression is illegal\n");
			prev = sym->u.cnst.i;
		} else if (tk == ',') {
			tk = get_tk();
			sym->u.cnst.i = ++prev;
		} else if (tk == '}')
			break;
		else
			error(-1, zcc_line_no, "illegal current input\n");
	}
	/* current token is '}'*/
	return;
}
#if 0
/*===----------------------------------------------------------------------------------
do not check the linkage conflict.
sel == chk_def, means it is definiton now, redifnition will report
sel == chk_decl, means it is only declaration
-----------------------------------------------------------------------------------===*/
static symbol *chk_define(sym_tab **st, type *ty, char *name, int lev, int sel) {
	symbol *sym = look_up_level(*st, name, lev);
	/*definiton*/
	if (sel == 0 /*chk_def*/) {
		if (sym && sym->is_defined) {
			error(-1, zcc_line_no, "redifinition of %s\n", name);
			return NULL;
		} else if (!sym) {
			sym = install(st, name, lev, lev>GLOBAL ? FUNC : PERM);
			sym->is_defined = 1;
			sym->ty = ty;
		/*symbol has been in symbol table, so need check the type is_compatiable*/
		} else
			if (is_compatiable(sym->ty, ty)) {
				complete_type(sym->ty, ty, sym->scope > GLOBAL ? TY : PERM);
				sym->is_defined = 1;
			} else {
				error(-1, zcc_line_no, "type conflict\n");
				return NULL;
			}
	/*declaration*/
	} else if (sel == 1 /*chk_decl*/) {
		if (sym == NULL) {
			sym = install(st, name, lev, lev > GLOBAL ? TY : PERM);
			sym->ty = ty;
		} else
			if (is_compatiable(sym->ty, ty))
				complete_type(sym->ty, ty, lev > GLOBAL ? TY : PERM);
			else {
				error(-1, zcc_line_no, "type conflict\n");
				return NULL;
			}
	} else
		assert(0);

	return sym;
}
#endif
/*===---------------------------------------------------------------------------------
enum-specifier
	enum [identifier] '{' {enumerator}+ '}'
enum type is not exist essentially. :-)
-----------------------------------------------------------------------------------===*/
static type *enum_spec() {
	char *name;
	symbol *sym = NULL;
	int define = 0;
	//char set[] = {0, 0};
	/*==---------------------------------------------------------
	1, enum '{' enumrators '}'
	2, enum tag '{' enumrators '}'
	3, enum tag
	4, enum tag; <-- ';' beyond the token flow of this function
	----------------------------------------------------------==*/
	assert(tk == TK_ENUM);
	tk = get_tk();
	if (tk == TK_ID) {
		tk = get_tk();
		name = lex_data.name;
	} else {
		//set[0] = '{';
		expect_tk_cur('{');
		name = gen_tmp_name("enum");
	}
	/* 1, enum { enumrators } 
	2, enum tag { enumrators } */
	if (tk == '{') {
		/// we can sure this is a enum type definition
		//set[0] = '}';
		sym = look_up_level(tags_tb, name, level);
		if (sym && sym->is_defined)
			error(-1, zcc_line_no, "enum %s have be defined in same scope\n", name);
		/* define enumrators*/
		tk = get_tk();
		e_define();
		expect_tk('}');
		define = 1;
	} else {
		/* 3, enum tag <--the current token is not ';'*/
		if (tk != ';' && (sym = look_up(tags_tb, name)) && sym->is_enum)
			return sym->ty;
	}
	/* 1/2, enum [tag] '{' enumrators '}' <--creat new enum type*/
	/* 4, enum tag; current token is ';' <-- need creat new incomplete enum type*/
	sym = install(&tags_tb, name, level, 0);
	sym->ty = int_type;
	sym->is_enum = 1;
	if (define)
		sym->is_defined = 1;
	
	return sym->ty;
}

/*===----------------------------------------------------------------------------------
declaration-specifier
	{storeage-class-specifier}
	{type-specifier}
	{type-quanlifier}
-----------------------------------------------------------------------------------===*/
type *specifier(int *sclass) {
	int _sign = 0;		//signed
	int _base = 0;		//base type
	int _length = 0;	//short or long?
	int _volatile = 0;
	int _const = 0;
	type *aty = NULL;	//struct/union specifier type
	type *ety = NULL;	//enum type
	type *tty = NULL;	//typedef type
	symbol *sym = NULL;
	type *rty;
	int temp = 0;

	if (sclass)
		*sclass = 0;
	else
		/* if sclass is NULL, means the caller ignores the storeage-class*/
		sclass = &temp;
	expect_first_cur(first_decl);
	while (is_first(tk, first_decl)) {
		switch (tk) {
		case TK_TYPEDEF:
		case TK_EXTERN:
		case TK_STATIC:
		case TK_AUTO:
		case TK_REGISTER:
			if (*sclass == 0)
				*sclass = tk;
			else
				error(-1, zcc_line_no, "type-specifier has duplicate %s\n", g_tk2str[tk]);
			tk = get_tk();
			continue;
		case TK_VOLATILE:
			if (_volatile == 0)
				_volatile = 1;
			else
				error(-1, zcc_line_no, "type-specifier has duplicate %s\n", g_tk2str[tk]);
			tk = get_tk();
			continue;
		case TK_CONST:
			if (_const == 0)
				_const = 1;
			else
				error(-1, zcc_line_no, "type-specifier has duplicate %s\n", g_tk2str[tk]);
			tk = get_tk();
			continue;
		case TK_SIGNED:
		case TK_UNSIGNED:
			if (_sign == 0)
				_sign = tk;
			else
				error(-1, zcc_line_no, "type-specifier has duplicate %s\n", g_tk2str[tk]);
			tk = get_tk();
			continue;
		case TK_SHORT:
		case TK_LONG:
			if (_length == 0)
				_length = TK_SHORT;
			else
				error(-1, zcc_line_no, "type-specifier has duplicate %s\n", g_tk2str[tk]);
			tk = get_tk();
			continue;
		case TK_STRUCT:
		case TK_UNION:
			aty = struct_spec();
			continue;
		case TK_ENUM:
			//tk = get_tk();
			ety = enum_spec();
			continue;
		case TK_INT:
		case TK_FLOAT:
		case TK_DOUBLE:
		case TK_VOID:
		case TK_CHAR:
			if (_base == 0)
				_base = tk;
			else
				error(-1, zcc_line_no, "type-specifier has duplicate %s", g_tk2str[tk]);
			tk = get_tk();
			continue;
		case TK_TYPE:
			tty = lex_data.ty;
			tk = get_tk();
			continue;
		/*if we cannot recognize the token*/
		default:
			break;
		}
	}//while (is_tspec(tk))
	/* well, now check everything we get*/
	if (_base == 0 && _length == 0 && _sign == 0)
		// must be struct/union/typedef
		if (aty && !ety && !tty)
			/*==-------------------------------------------------------------------------
			struct/union type, maybe shared by declarators more than one
			Attention pass qualifiers from most outside type to object, so call qualify
				const struct s {} cs; //const struct type
				struct s ncs; //no-const struct type
			call qulify() can santisfy this constrain of type access 
			--------------------------------------------------------------------------==*/
			return qualify(aty, _const, _volatile, level > GLOBAL ? TY : PERM);
		else if (!aty && ety && !tty)
			/* enum type, int, just ignore the qualifiers for enum type*/
			return int_type;
		else if (!aty && !ety && tty) {
			/* typedef type, must call qualify(), because tty maybe shared*/
			if (_const && tty->is_const || _volatile && tty->is_volatile)
				error(-1, zcc_line_no, "same type qualifiers more than once\n");
			return qualify(tty, _const, _volatile, level > GLOBAL ? TY : PERM);
		} else
			error(-1, zcc_line_no, "type-specifier error\n");
		
	/*storeage-class-value will return, the caller will decide which area type will 
	be allocated and do a totally copy, the data allocated here will be uesless,
	so we are safe allocate in area FUNC here*/
	if (_base == TK_VOID && _length == 0 && _sign == 0)
		rty = void_type;
	else if (_base == TK_CHAR) {
		if (_sign == TK_SIGNED || _sign == 0)
			rty = char_type;
		else if (_sign = TK_UNSIGNED)
			rty = unsigned_char;
	} else if (_length == TK_SHORT) {
		/* signed short class*/
		if (_base == 0 && _sign == 0 || _base == 0 && _sign == TK_SIGNED ||
			_base == TK_INT && _sign == 0 || _base == TK_INT && _sign == TK_SIGNED)
			rty = short_type;
		/* unsigned short class*/
		else if (_sign == TK_UNSIGNED && (_base == 0 || _base == TK_INT))
			rty = unsigned_short;
	} else if (_sign == TK_UNSIGNED) {
		if (_base == 0 && _length == 0 || _base == TK_INT && _length == 0 ||
			_base == 0 && _length == TK_LONG || _base == TK_INT && _length == TK_LONG)
			rty = unsigned_int;
	} else if (_base == TK_FLOAT && _sign == 0 && _length == 0)
		rty = float_type;
	else if (_base == TK_DOUBLE && _sign == 0 && (_length == 0 || _length == TK_LONG))
		rty = double_type;
	else if (_base == TK_INT && _sign == 0 && _length == 0 ||	/* int*/ 
			_base == TK_INT && _sign == TK_SIGNED && _length == 0 ||	/* signed int*/
			_base == 0 && _sign == TK_SIGNED && _length == 0 ||	/* signed*/
			_base == 0 && _sign == 0 && _length == TK_LONG ||		/* long*/
			_base == 0 && _sign == TK_SIGNED && _length == TK_LONG ||	/* signed long*/
			_base == TK_INT && _sign == 0 && _length == TK_LONG ||	/* long int*/
			_base == TK_INT && _sign == TK_SIGNED && _length == TK_LONG)	/* signed long int*/
		rty = int_type;
	else
		error(-1, zcc_line_no, "type-specifier error\n");
	/* add qualifier*/
	if (_const || _volatile)
		/* base type, must call qualify()*/
		return qualify(rty, _const, _volatile, level > GLOBAL ? TY : PERM);
	return rty;
#if 0
	NEW0(rty, FUNC);
	rty->is_const = _const;
	rty->is_volatile = _volatile;
	
	if (_sign = TK_SIGNED) {
		if (_length == TK_SHORT) {
			/*signed short [int]*/
			if (_base == TK_INT || _base == 0)
				rty->cat = TK_SHORT;
			else
				goto err;
		} else if (_length == TK_LONG) {
			/*signed long [int]*/
			if (_base == TK_INT || _base == 0)
				rty->cat = TK_INT;
			else
				goto err;
		} else if (_length == 0) {
			/*signed ([long]|[short]|[int]|[char])*/
			if (_base == TK_INT || _base == 0 || _base == TK_LONG)
				rty->cat = TK_INT;
			else if (_base == TK_SHORT)
				rty->cat = TK_SHORT;
			else if (_base == TK_CHAR)
				rty->cat = TK_CHAR;
			else
				goto err;
		}
	} else if (_sign == TK_UNSIGNED) {
		if (_length == TK_SHORT) {
			/*unsigned short [int]*/
			if (_base == TK_INT || _base == 0) {
				rty->cat = TK_SHORT;
				rty->is_unsigned = 1;
			} else
				goto err;
		} else if (_length == TK_LONG) {
			/* unsigned long [int]*/
			if (_base == TK_INT || _base == 0) {
				rty->cat = TK_INT;
				rty->is_unsigned = 1;
			} else
				goto err;
		} else if (_length == 0) {
			/*unsigned ([long]|[short]|[int]|[char])*/
			rty->is_unsigned = 1;
			if (_base == TK_INT || _base == 0 || _base == TK_LONG)
				rty->cat = TK_INT;
			else if (_base == TK_SHORT)
				rty->cat = TK_SHORT;
			else if (_base == TK_CHAR)
				rty->cat = TK_CHAR;
			else
				goto err;
		}
	} else if (_sign == 0) {
		if (_length == TK_SHORT) {
			/* short [int]*/
			if (_base == TK_INT || _base == 0)
				rty->cat = TK_SHORT;
			else
				goto err;
		} else if (_length == TK_LONG) {
			/* long [int]*/
			if (_base == TK_INT || _base == 0)
				rty->cat = TK_INT;
			else
				goto err;
		} else if (_length == 0) {
		
		}
	} else
		goto err;

	return rty;

err:
#endif
	assert(0);
}

/*===------------------------------------------------------------------------------------

------------------------------------------------------------------------------------===*/
void decl_local() {
	int sclass;
	symbol *sym, *prev = NULL;
	char *name;
	type *dty = NULL, *bty = NULL;
	//type *bty;

	/* local declaration, assign its stack offset*/
	assert(level > GLOBAL);
	expect_first_cur(first_decl);
	bty = specifier(&sclass);
	while (1) {
		assert(bty);
		dty = dclr(bty, &name, NULL);
		prev = look_up(ids_tb, name);
		/* every thing is possible for local variable*/
		/* make this outside the next switch, otherwise will break jump table*/
		if (sclass == 0 || sclass == TK_REGISTER) {
			if (prev) {
				if (dty->cat == CAT_FUNCTION) {
					/* register x funtion x block*/
					if (sclass == TK_REGISTER)
						error(-1, zcc_line_no, "register storeage class is illegal in function definition\n");
					/* block function have not storeage class, likes it has extern*/
					if (!is_compatiable(prev->ty, dty))
						error(-1, zcc_line_no, "type conflict with previous function: %s\n", prev->name);
				} else {
					/* block object have not storeage class must has no linkage*/
					/* no/register x object x block*/
					if (prev->scope == level && prev->is_no_linkage)
						error(-1, zcc_line_no, "on linkage identifier: %s more than one\n", prev->name);
				}
			} else {
				sym = install(&ids_tb, name, level, FUNC);
				sym->is_defined = 1;
				sym->is_no_linkage = 1;
				sym->ty = dty;
				/*
				if (dty->size == 0)
						error(-1, zcc_line_no,
						"identifier: %s has no linkage can not has incomplete type\n", name);
				alloc_stack(sym);
				*/
			}
		} else
			/* sclass != 0*/
			switch (sclass) {
			case TK_TYPEDEF:
				if (prev) {
					if (prev->scope == level)
						error(-1, zcc_line_no, "typedef name: %s have be defined\n", prev->name);
				} else {
					sym = install(&ids_tb, name, level, FUNC);
					sym->is_typedef = 1;
					sym->is_defined = 1;
					sym->ty = dty;
					// alloc_stack will ignore this symbol
					sym->ebp_off = 1;
				}
				break;
			case TK_EXTERN:
				/* C89 TR1*/
				if (prev && prev->is_no_linkage == 0/* prev = look_up(globals_tb, name)*/) {
					/* prev has linkage*/
					if (is_compatiable(prev->ty, dty))
						complete_type(prev->ty, dty, PERM);
					else
						error(-1, zcc_line_no, "type conflict with previous: %s\n", name);
				} else {
					/* if there is a global declaration, it's scope will be overwrite by GLOBAL*/
					sym = install(&ids_tb, name, level, PERM);
					sym->is_external = 1;
					//sym->is_extern = 1;
					sym->ty = dty;
					// alloc_stack will ignore this symbol
					sym->ebp_off = 1;
					/* output in .comm/.data/.global accroding to is_defined/is_external/is_no_linkage*/
					//add2slist_head(&global_ids, sym, PERM);
				}
				break;
			case TK_STATIC:
				if (prev) {
					if (bty->cat == CAT_FUNCTION)
						error(-1, zcc_line_no,
						"function: %s can not has illegal storeage class in block scope\n", name);
					else {
						/* no linkage identifier, so we test whether or not they are in same block,
						Attention the scope rules*/
						if (prev->scope == level && prev->is_no_linkage)
							error(-1, zcc_line_no, "on linkage identifier: %s more than one\n", name);
					}
				} else {
					/* obey the scope rule, but allocation in .data/.lcomm*/
					sym = install(&ids_tb, name, level, PERM);
					sym->is_no_linkage = 1;
					sym->is_defined = 1;
					/// need this, this object is in .bss
					sym->is_static = 1;
					sym->ty = dty;
					/* name mangling*/
					sym->asm_name = gen_tmp_name(sym->name);
					add2slist_head(&_F.data_ids, sym, PERM);
				}
				break;
			case TK_AUTO:
					if (prev) {
					if (bty->cat == CAT_FUNCTION)
						error(-1, zcc_line_no,
						"function: %s can not has illegal storeage class in block scope\n", name);
					else {
						if (prev->scope == level && prev->is_no_linkage)
							error(-1, zcc_line_no, "on linkage identifier: %s more than one\n", name);
					}
				} else {
					sym = install(&ids_tb, name, level, FUNC);
					sym->is_no_linkage = 1;
					sym->is_defined = 1;
					sym->ty = dty;
				}
				break;
			default:
				assert(0);
			}
		/// sclass if else over
		if (sym->ty->cat == CAT_FUNCTION &&
			(sym->ty->bty->cat == CAT_ARRAY || sym->ty->bty->cat == CAT_FUNCTION))
			error(-1, zcc_line_no, "function return array or function\n");
		/// token flow after declarator
		if (tk == '=') {
			/* in block scope, identifier has linkage should has no initializer*/
			if (sym->is_external)
				error(-1, zcc_line_no, "%s should not has initializer\n", sym->name);
			tk = get_tk();
			sym->iz = initializer();
			if (sym->is_static == 0)
				// static variable has be add to data_ids
				add2slist_tail(&_F.stack_inits, sym, FUNC);
		}
		
		if (tk == '{')
			error(-1, zcc_line_no, "illegal token: '{'\n");
		else if (tk == ',') {
			tk = get_tk();
			if (sym->is_no_linkage && sym->ty->size == 0)
				error(-1, zcc_line_no, "identifier: %s, no linkage but incomplete\n", name);
		} else
			expect_cur(follow_tk_decl);
		/// alloc memory at stack
		alloc_stack(sym);
		if (tk == ';') {
			//clear_qualifier(dty);
			tk = get_tk();
			/* this declaration satement is over*/
			break;
		}
	}

	return;
}

/*===------------------------------------------------------------------------------------
parameter-list
	{specifier declarator|abstract-declarator ,}
------------------------------------------------------------------------------------===*/
void parameter(dlist **params) {
	type *ty;
	int sclass = 0;
	char *name = NULL;
	int params_number = 0;
	//char set[] = {')', 0};
	int is_va = 0;

	/* special deal with the first parameter*/
	assert(tk != ')');
	assert(*params == NULL);
	expect_first_cur(first_decl);
	if (tk != TK_ELPS)
		ty = specifier(&sclass);
	else
		error(-1, zcc_line_no, "must has one no-variant parameter at least before the ... parameter\n");
	if (sclass != TK_REGISTER)
		error(-1, zcc_line_no, "in parameter declaration the storage only can be register\n");
	/* func(void)*/
	if (ty->cat == CAT_VOID) {
		add2dlist_tail(params, gen_symbol(name, ty, PERM), PERM);
		expect_tk_cur(')');
		return;
	}
	if (tk != ',' && tk != ')')
		ty = dclr(ty, &name, NULL);
	/* in parameter declaration, the storeage class must be register,
	arguments pass through stack, so i just ignore it in this version of zcc*/
	add2dlist_tail(params, gen_symbol(name, ty, PERM), PERM);
	/* TODO, ellipsis!!!!!!!!!!!!!!*/
	while (tk == ',') {
		tk = get_tk();
		sclass = 0;
		if (tk != TK_ELPS)
			ty = specifier(&sclass);
		else {
			tk = get_tk();
			is_va = 1;
			ty = new_type(CAT_VA, NULL, 0, 0, PERM);
		}
		if (sclass != TK_REGISTER)
			error(-1, zcc_line_no, "in parameter declaration the storage only can be register\n");
		if (tk != ',' && tk != ')') {
			/* clear previous data*/
			name = NULL;
			ty = dclr(ty, &name, NULL);
		}
		add2dlist_tail(params, gen_symbol(name, ty, PERM), PERM);
		if (tk == ')' || is_va)
			break;
	}
	expect_tk_cur(')');
	
	return;
}

/*===-------------------------------------------------------------------------------------

--------------------------------------------------------------------------------------===*/
static type *reverse_type(type *head) {
	type *t1 = head, *t2 = head->bty, *t3;

	assert(head);
	t1->bty = NULL;	/* do not miss this*/
	while (t1 && t2) {
		t3 = t2->bty;
		t2->bty = t1;
		t1 = t2;
		t2 = t3;
	}
	/* if head has more than one node*/
	if (t2)
		head =t2;

	return head;
}

/*===-------------------------------------------------------------------------------------

-------------------------------------------------------------------------------------===*/
static void array_size(type *head) {
	//int size;
	type *t1 = head, *t2 = head->bty;
	if (head->cat != CAT_ARRAY)
		return;
	head = reverse_type(head);
	t1 = head;
	/* at least have type specifier*/
	assert(t1->cat != CAT_ARRAY);
	t2 = head->bty;
	while (t1 && t2) {
		if (t2->cat == CAT_ARRAY) {
			/* type of element can not be function*/
			if (t1->size == 0)
				error(-1, zcc_line_no, "the size of array's element is 0\n");
			t2->align = t1->size;
			t2->size = t2->u.a.n * t1->size;
			t1 = t2;
			t2 = t2->bty;
		}
	}
	head = reverse_type(head);
	
	return;
}

/*===------------------------------------------------------------------------------------
direct-declarator
	identifier|'('declarator')' {'['constant-expression']'} {'('[parameter-slist]')'}
------------------------------------------------------------------------------------===*/
static type *d_dclr(type *bty, char **name) {
	type *head = NULL, *dd = NULL;
	type *patch = NULL;	/* must be completed in this function, because of the recursion*/
	//char set[2] = {0,};
	ast *tree;
	{
		char set[] = {TK_ID, '(', '[', 0};
		expect_cur(set);
	}
	if (tk == TK_ID) {
		/* identifier*/
		*name = lex_data.name;
		tk = get_tk();
	} else if (tk == '(') {
		/* '(' declarator ')'*/
		tk = get_tk();
		head = dclr(NULL, name, &patch);
		assert(patch->bty == NULL);
		//set[0] = ')';
		expect_tk(')');
	}
	while (1) {
		if (tk == '(') {
			// eat current token '('
			tk = get_tk();
			if (dd)
				/* function must has linkage*/
				dd->bty = new_type(CAT_FUNCTION, NULL, 0, 0, PERM);
			else
				dd = new_type(CAT_FUNCTION, NULL, 0, 0, PERM);
			if (patch && patch->bty == NULL)
				patch->bty = dd;	/* complete the type list*/
			if (head == NULL)
				head = dd;
			if (tk != ')') {
				tk = get_tk();
				/* the function parameter() will eat the last ')'*/
				parameter(&dd->u.f.params_decl);
			} else
				assert(dd->u.f.params_decl == NULL);
			//set[0] = ')';
			expect_tk(')');
			//dd = dd->bty;
		} else if (tk == '[') {
			// eat the current token '['
			tk = get_tk();
			if (dd)
				dd->bty = new_type(CAT_ARRAY, NULL, 0, 0, 0);
			else
				dd = new_type(CAT_ARRAY, NULL, 0, 0, 0);
			if (patch && patch->bty == NULL)
				patch->bty = dd;	/* complete the type slist*/
			if (head == NULL)
				head = dd;
			if (tk != ']') {
				expect_first_cur(first_cnst);
				tree = cnst_expr();
				need_type(tree, CAT_INT);
				dd->u.a.n = tree->u.cnst->u.cnst.i;
			} else
				tk = get_tk();
			//set[0] = ']';
			expect_tk(']');
			//dd = dd->bty;
		} else
			break;
	}
	if (dd)
		dd->bty = bty;
	if (head)
		array_size(head);
	else
		head = bty;

	return head;
}

/*===------------------------------------------------------------------------------------
declarator
	{[* [qualifier-slist]]} direct-declarator
if argument for bty is NULL, patch must not NULL
-------------------------------------------------------------------------------------===*/
static type *dclr(type *bty, char **name, type **patch_addr) {
	type *ty;
	char set[] = {'*', TK_ID, '(', 0};
	int dirty = 0;

	expect_cur(set);
	/* initialize loop*/
	ty = bty;
	while (tk == '*') {
		//int i;
		int _const = 0, _volatile = 0;
		tk = get_tk();
		/* we just use the bty, don't clone it, make sure specifier don't be called 
		before dclr is over*/
		ty = pointer(ty, 0);
		/* const volatile*/
		set[0] = fsv_qual;
		set[1] = 0;
		while (is_first(tk, set)) {
			switch (tk) {
			case TK_CONST:
				if (_const == 0)
					_const = 1;
				else
					error(-1, zcc_line_no, "type qualifier: const more than once\n");
				/* these qualifiers are for pointer which is part of declarater, 
				is unique for object, impossible be shared, so we can do this*/
				ty->is_const = 1;
				break;
			case TK_VOLATILE:
				if (_volatile == 0)
					_volatile = 1;
				else
					error(-1, zcc_line_no, "type qualifier: volatile more than once\n");
				ty->is_volatile = 1;
				break;
			}
			tk = get_tk();
		}
		//ty = qualify(ty, _const, _volatile, 0);
		if (bty == NULL && dirty == 0) {
			/* if bty is NULL, we can not sure point to what, so need patch this later*/
			/* should has other patch point, int (func(int)) ()*/
			*patch_addr = ty;		/* patch point to the type whose bty need be assigned the sub-type*/
			dirty = 1;
		}
	} /// while(tk == '*')
	if (bty == NULL && dirty == 0)
		ty = *patch_addr = d_dclr(bty, name);
	else
		ty = d_dclr(bty, name);

	return ty;
}

/*===-----------------------------------------------------------------------------------
external-declaration
	specifier {declarator [= initializer (,|;)]}+ [compound-statement]
only be called by main42 - the entry
------------------------------------------------------------------------------------===*/
static void decl_global() {
	int sclass = 0;
	symbol *sym, *prev = NULL;
	char *name;
	type *dty = NULL, *bty = NULL;

	expect_first_cur(first_decl);
	bty = specifier(&sclass);
	assert(level == GLOBAL);
	/* global declaration, include function definiton*/ 
	while (1) {
		assert(bty);
		dty = dclr(bty, &name, NULL);
		prev = look_up_level(globals_tb, name, GLOBAL);
		/* sclass has one of storeage class: extern, static, typedef*/
		/* global identifiers must have linkage*/
		/* make this outside the next switch, otherwise will break jump table*/
		if (sclass == 0) {
			if (prev) {
				if (dty->cat == CAT_FUNCTION) {
					/* (C89)global function have not storeage class, likes it has extern
					zcc did not compelete the function type, just report error*/
					if (is_compatiable(prev->ty, dty) == 0)
						error(-1, zcc_line_no, "type conflict with previous: %s\n", prev->name);
				} else {
					/* (C89)global object have not storeage class must be external linkage*/
					if (prev->is_external && is_compatiable(prev->ty, dty))
						complete_type(prev->ty, dty, PERM);
					else
						error(-1, zcc_line_no, "linkage conflict with previous: %s\n", prev->name);
				}
			/* no previous symbol*/
			} else {
				sym = install(&globals_tb, name, GLOBAL, PERM);
				sym->is_external = 1;
				sym->ty = dty;
				/* output in .comm/.data/.global accroding to is_defined/is_external/is_no_linkage*/
				if (sym->ty->cat != CAT_FUNCTION)
					add2slist_head(&_F.data_ids, sym, PERM);
			}
		} else
			/* sclass != 0*/
			switch (sclass) {
			case TK_TYPEDEF:
				if (prev)
					/* typedef name on linkage, same scope should has only one*/
					error(-1, zcc_line_no, "typedef no linkage, name: %s have be defined\n", prev->name);
				else {
					sym = install(&globals_tb, name, GLOBAL, PERM);
					sym->is_typedef = 1;
					sym->is_defined = 1;
					sym->ty = dty;
				}
				break;
			case TK_EXTERN:
				if (prev) {
					/* previous symbol's linkage must be external or internal*/
					if (is_compatiable(prev->ty, dty))
						/* maybe this function type*/
						complete_type(prev->ty, dty, PERM);
					else
						error(-1, zcc_line_no, "type conflict with previous: %s\n", prev->name);
				} else {
					sym = install(&globals_tb, name, GLOBAL, PERM);
					sym->is_external = 1;
					//sym->is_extern = 1;
					sym->ty = dty;
					/* output in .comm/.data/.global accroding to is_defined/is_external/is_no_linkage*/
					//add2slist_head(&global_ids, sym, PERM);
				}
				break;
			case TK_STATIC:
				if (prev) {
					if (prev->is_internal && is_compatiable(prev->ty, dty))
						complete_type(prev->ty, dty, PERM);
					else
						error(-1, zcc_line_no, "type conflict with previous: %s\n", prev->name);
				} else {
					sym = install(&globals_tb, name, GLOBAL, PERM);
					sym->is_internal = 1;
					sym->ty = dty;
					/* output in .comm/.data/.global accroding to is_defined/is_external/is_no_linkage*/
					/* add internal identifier at the tail of list*/
					if (sym->ty->cat != CAT_FUNCTION)
						add2slist_tail(&_F.data_ids, sym, PERM);
				}
				break;
			case TK_AUTO:
			case TK_REGISTER:
				error(-1, zcc_line_no,
					"auto and register shall not appear at external declaration\n");
				break;
			default:
				assert(0);
			} ///switch(sclass)
		/// sclass if else over
		if (sym->ty->cat == CAT_FUNCTION &&
			(sym->ty->bty->cat == CAT_ARRAY || sym->ty->bty->cat == CAT_FUNCTION))
			error(-1, zcc_line_no, "function return array or function\n");
		/// token flow after declarator
		if (tk == '=') {
			if (sym->is_defined)
				error(-1, zcc_line_no, "variable: %s redefinition\n", sym->name);
			tk = get_tk();
			sym->is_defined = 1;
			sym->iz = initializer();
			//add2slist_tail(&_F.data_inits, sym, FUNC);
		} 
		if (tk == '{') {
			/* function definiton*/
			_F.cur_func = sym;
			func_define();
			break;
		} else if (tk == ';') {
			/* eat this ';'*/
			tk = get_tk();
			break;
		} else if (tk == ',')
			/* must be ',', eat it*/
			tk = get_tk();
		else
			expect_cur(follow_tk_decl);	// {'=', '{', ';', ',', 0}
	}

	return;
}

/*===----------------------------------------------------------------------------------
this data used by icode generation
-----------------------------------------------------------------------------------===*/
void init_spill_loc() {
	_F.spill_i32.ty = int_type;
	alloc_stack(&_F.spill_i32);
	_F.spill_f32.ty = float_type;
	alloc_stack(&_F.spill_f32);
	_F.spill_f64.ty = double_type;
	alloc_stack(&_F.spill_f64);

	return;
}

/*===-----------------------------------------------------------------------------------
the entrance of parser, analyse the syntax	:
translation-unit
		external-declaration {external-declaration}
the zcc shrinks the semantics of standard C, the function-definition must be start with
	declaration-specifiers, it's not option. and will not accept funtion definition of 
	the K&R style. I think this is helpful for the coding clear. :-)
------------------------------------------------------------------------------------===*/
void main_42() {
	tk = get_tk();
	while (tk != TK_EOF) {
		/* external-declaration including deal with functions*/
		if (is_first(tk, first_decl))
			/*external-declaration*/
			decl_global();
		else {
			expect_first_cur(first_decl);	
			error(-1, zcc_line_no, "illegal character\n");
		}
		/* global empty declaration*/
		if (tk == ';')
			error(-1, zcc_line_no, "global empty declaration\n");
	}

	return;
}
