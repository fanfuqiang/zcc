/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

//static int _zcc_init_types;
/* zcc internal type*/
type *void_type;
type *c_void_type;
type *v_void_type;
type *cv_void_type;
type *char_type;
type *c_char_type;
type *v_char_type;
type *cv_char_type;
type *unsigned_char;
type *short_type;
type *unsigned_short;
type *int_type;
type *c_int_type;
type *v_int_type;
type *cv_int_type;
type *unsigned_int;
type *pointer_type;
type *float_type;
type *c_float_type;
type *v_float_type;
type *cv_float_type;
type *double_type;
type *c_double_type;
type *v_double_type;
type *cv_double_type;
/* base type table*/
static type *base_type_tab[5][4];
/**
= {
	{void_type, c_void_type, v_void_type, cv_void_type},
	{char_type, c_char_type, v_char_type, cv_char_type},
	{int_type, c_int_type, v_int_type, cv_int_type},
	{float_type, c_float_type, v_float_type, cv_float_type},
	{double_type, c_double_type, v_double_type, cv_double_type}
};
*/
/*===--------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
type *new_type(int cat, type *ty, int size, int align, int area) {
	type *p;
	if (align == 0)
		align = size;
	if (area <= 0)
		area = level > GLOBAL ? TY : PERM;
	
	NEW0(p, area);
	p->cat = cat;
	p->bty = ty;
	p->size = size;
	p->align = align;

	return p;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void init_types() {
	int i = 0, j = 0;

#define init(t, cat, size) t = new_type(cat, NULL, size, size, PERM);\
	base_type_tab[i][j] = t;\
	i++; j++;\
	i = i / 4;

	//t->sym = s = install(&types_tb, name, GLOBAL, PERM); s->ty = t
	/* void*/
	init(void_type, CAT_VOID, 0);
	init(c_void_type, CAT_VOID, 0);
	init(v_void_type, CAT_VOID, 0);
	init(cv_void_type, CAT_VOID, 0);
	/* char*/
	init(char_type, CAT_CHAR, 1);
	init(c_char_type, CAT_CHAR, 1);
	init(v_char_type, CAT_CHAR, 1);
	init(cv_char_type, CAT_CHAR, 1);
	/* int*/
	init(int_type, CAT_INT, 4);
	init(c_int_type, CAT_INT, 4);
	init(v_int_type, CAT_INT, 4);
	init(cv_int_type, CAT_INT, 4);
	/* float*/
	init(float_type, CAT_FLOAT, 4);
	init(c_float_type, CAT_FLOAT, 4);
	init(v_float_type, CAT_FLOAT, 4);
	init(cv_float_type, CAT_FLOAT, 4);
	/* double*/
	init(double_type, CAT_DOUBLE, 8);
	init(c_double_type, CAT_DOUBLE, 8);
	init(v_double_type, CAT_DOUBLE, 8);
	init(cv_double_type, CAT_DOUBLE, 8);
#undef init

	unsigned_char = char_type;
	short_type = int_type;
	unsigned_short = int_type;
	unsigned_int = int_type;
	/* attach type qualifiers*/
	c_void_type->is_const = 1;
	v_void_type->is_volatile = 1;
	cv_void_type->is_const = 1;
	cv_void_type->is_volatile = 1;
	c_char_type->is_const = 1;
	v_char_type->is_volatile = 1;
	cv_char_type->is_const = 1;
	cv_char_type->is_volatile = 1;
	c_int_type->is_const = 1;
	v_int_type->is_volatile = 1;
	cv_int_type->is_const = 1;
	cv_int_type->is_volatile = 1;
	c_float_type->is_const = 1;
	v_float_type->is_volatile = 1;
	cv_float_type->is_const = 1;
	cv_float_type->is_volatile = 1;
	c_double_type->is_const = 1;
	v_double_type->is_volatile = 1;
	cv_double_type->is_const = 1;
	cv_double_type->is_volatile = 1;
	/* pointer*/
	pointer_type = new_type(CAT_POINTER, NULL, 4, 4, PERM);

	return;
}

/*===----------------------------------------------------------------------------
if is compatiable parameter type return 1
----------------------------------------------------------------------------===*/
static int is_cps(type *ty1, type *ty2) {
	assert(ty1->cat < CAT_FUNCTION && ty2->cat < CAT_FUNCTION);
	if (ty1 == ty2)
		return 1;
	if (ty1->cat == ty2->cat) {
		switch (ty1->cat) {
		case CAT_VOID:
		case CAT_CHAR:
		case CAT_UCHAR:
		case CAT_SHORT:
		case CAT_USHORT:
		case CAT_INT:
		case CAT_UINT:
		case CAT_FLOAT:
		case CAT_DOUBLE:
		case CAT_VA:
			return 1;
		case CAT_POINTER:
			return is_compatiable(ty1->bty, ty2->bty);
		case CAT_STRUCT:
		case CAT_UNION:
			/* specifier() => qualify(aty)*/
			if (ty1->u.s.sl == ty2->u.s.sl)
				return 1;
			/* TODO: test every member is not compatiable*/
			return 0;
		case CAT_ARRAY:
			/* actually is a pointer, size of array ignored*/
			return is_compatiable(ty1->bty, ty2->bty);
		default:
			assert(0);
		}
	}

	return 0;
}

/*===----------------------------------------------------------------------------
if two type compatiable return 1, otherwise return 0
-----------------------------------------------------------------------------===*/
int is_compatiable(type *ty1, type *ty2) {
	if (ty1 == ty2)
		return 1;
	if (ty1->is_const == ty2->is_const && ty1->is_volatile == ty2->is_volatile
		&& ty1->cat == ty2->cat) {
		if (is_arith_cat(ty1->cat))
			assert(!"Panic, base arithmetic type did not be shared");
		switch (ty1->cat) {
		case CAT_POINTER:
			return is_compatiable(ty1->bty, ty2->bty);
		case CAT_STRUCT:
		case CAT_UNION:
			/* struct/union type is scope relatived*/
			if (ty1->sym->name == ty2->sym->name
				&& ty1->sym->scope == ty2->sym->scope)
				return 1;
			else
				return 0;
		case CAT_ARRAY:
			/* if one array is unknown size*/
			if (ty1->size == 0 || ty2->size == 0 || ty1->size == ty2->size)
				return is_compatiable(ty1->bty, ty2->bty);
			return 0;
		case CAT_FUNCTION:
		{
			dlist *head1, *p1, *head2, *p2;
			p1 = head1 = ty1->u.f.params_decl;
			p2 = head2 = ty2->u.f.params_decl;
			if (head1 == NULL && head2 == NULL)
				return 1;
			if (head1 && head2) {
				do {
					if (!is_cps(((symbol *)(p1->it))->ty, ((symbol *)(p2->it))->ty))
						return 0;
					p1 = p1->next;
					p2 = p2->next;
				} while (p1 != head1 && p2 != head2);
				if (p1 == head1 && p2 == head2)
					return 1;
			}
			return 0;
		}
		default:
			assert(!"Panic, type category error");
		}
	}

	return 0;
}

/*===----------------------------------------------------------------------------

-----------------------------------------------------------------------------===*/
int is_cpt_no_qual(type *ty1, type *ty2) {
	if (ty1 == ty2)
		return 1;
	if (ty1->cat == ty2->cat) {
		if (is_arith_cat(ty1->cat))
			assert(!"Panic, base arithmetic type did not be shared");
		switch (ty1->cat) {
		case CAT_POINTER:
			return is_compatiable(ty1->bty, ty2->bty);
		case CAT_STRUCT:
		case CAT_UNION:
			/* struct/union type is scope relatived*/
			if (ty1->sym->name == ty2->sym->name
				&& ty1->sym->scope == ty2->sym->scope)
				return 1;
			else
				return 0;
		case CAT_ARRAY:
			/* if one array is unknown size*/
			if (ty1->size == 0 || ty2->size == 0 || ty1->size == ty2->size)
				return is_compatiable(ty1->bty, ty2->bty);
			return 0;
		case CAT_FUNCTION:
		{
			dlist *head1, *p1, *head2, *p2;
			p1 = head1 = ty1->u.f.params_decl;
			p2 = head2 = ty2->u.f.params_decl;
			if (head1 == NULL && head2 == NULL)
				return 1;
			if (head1 && head2) {
				do {
					if (!is_cps(((symbol *)(p1->it))->ty, ((symbol *)(p2->it))->ty))
						return 0;
					p1 = p1->next;
					p2 = p2->next;
				} while (p1 != head1 && p2 != head2);
				if (p1 == head1 && p2 == head2)
					return 1;
			}
			return 0;
		}
		default:
			assert(!"Panic, type category error");
		}
	}

	return 0;
}

/*===----------------------------------------------------------------------------
complete the first type with the second type's data
----------------------------------------------------------------------------===*/
void complete_type(type *dst, type *src, int area) {
	assert(dst);
	assert(dst->cat == src->cat);
	if (area == 0)
		area = level > GLOBAL ? TY : PERM; 
	switch (dst->cat) {
	case CAT_STRUCT:
	case CAT_UNION:
	case CAT_ARRAY:
		/* Attention: copy content but not pointer assign,
		original incomplete type may has been referenced*/
		memcpy(dst, src, sizeof(dst));
		break;
	case CAT_FUNCTION:
		break;
	default:
		assert(!"Panic, internal type system error");
	}

	return;
}
#if 0
/*===---------------------------------------------------------------------------
this is very rough, do not have same type item shares.
	flag = 3, clone even the base type
	flag = 2, deep clone
	flag = 1, clone type except base type
	flag = 0, skin clone
----------------------------------------------------------------------------===*/
static type *clone_type_int(type *ty, int flag, int area) {
	type **pp, *head = NULL;
	
	assert(ty);
	pp = &head;
	do {
		switch (ty->cat) {
		case CAT_VOID:
		case CAT_CHAR:
		case CAT_SHORT:
		case CAT_INT:
		case CAT_FLOAT:
		case CAT_DOUBLE:
			/* this is internal type can be shared, allocated in PERM*/
			if (ty->is_const || ty->is_volatile) {
				NEW0(*pp, area);
				memcpy(*pp, ty, sizeof(*ty));
			} else {
				*pp = ty;
			}
			assert((*pp)->bty == NULL);
			break;
		case CAT_STRUCT:
		case CAT_UNION:
		case CAT_ARRAY:
		case CAT_FUNCTION:
		/* invalidated*/
		//case CAT_ENUM:
		case CAT_SCALAR:
		case CAT_VA:
		default:
			assert(0);
		}

		ty = ty->bty;
		pp = &(*pp)->bty;
	} while (flag && ty);

	return head;
}

/*===-----------------------------------------------------------------------------

------------------------------------------------------------------------------===*/
type *clone_type_deep(type *ty, int area) {
	return clone_type_int(ty, 2, area);
}

/*===-----------------------------------------------------------------------------

------------------------------------------------------------------------------===*/
type *clone_type(type *ty, int area) {
	return clone_type_int(ty, 1, area);
}

/*===-----------------------------------------------------------------------------
just clone the most outside type item
------------------------------------------------------------------------------===*/
type *clone_type_skin(type *ty) {
	return clone_type_int(ty, 0, FUNC);
}
#endif
/*===-----------------------------------------------------------------------------

------------------------------------------------------------------------------===*/
static type *mem_copy_type(type *src, int area) {
	type *dst;

	NEW0(dst, area);
	memcpy(dst, src, sizeof(dst));

	return dst;
}

/*===-----------------------------------------------------------------------------
this function is the essential of type item share among different symbols
------------------------------------------------------------------------------===*/
type *qualify(type *ty, int is_const, int is_volatile, int area) {
	type *ty1 = ty, *ty2;

	assert(ty->cat < CAT_FUNCTION);
	/* detials*/
	if (area <= 0)
		area = level > GLOBAL ? TY : PERM;
	is_const = is_const ? 1 : 0;
	is_volatile = is_volatile ? 2 : 0;
	/* easy time, qualifiers same with original type*/
	if (ty->cat != CAT_ARRAY) {
		if (ty->is_const == is_const && ty->is_volatile == is_volatile ? 1 : 0)
			return ty;
	} else {
		/* qual(array(element)) == array(qual(element))*/
		for (ty2 = ty; ty2 && ty2->cat == CAT_ARRAY; ty2 = ty2->bty)
			;
		assert(ty2 && "array element is NULL");
		if (ty2->is_const == is_const && ty2->is_volatile == is_volatile ? 1 : 0)
			return ty;
	}
	/* hard time*/
	if (is_arith_cat(ty->cat)) {
		int row, column;
		column = is_const + is_volatile;
		assert(column < 4);
		switch (ty->cat) {
			/* will be a jump-table*/
		case CAT_VOID:
			row = 0;
		case CAT_CHAR:
		case CAT_UCHAR:
			row = 1;
		case CAT_SHORT:
		case CAT_USHORT:
		case CAT_INT:
		case CAT_UINT:
			row = 2;
		case CAT_FLOAT:
			row = 3;
		case CAT_DOUBLE:
			row = 4;
		}
		return base_type_tab[row][column];
	}
	/*==--------------------------------------
	qual(array(ty)) = array(qual(ty))
	typedef int A[5];
	const A a5;
	A maybe shared with other identifier
	---------------------------------------==*/
	/* use recursion can deal with array qualify too*/
	if (ty->cat == CAT_ARRAY) {
		ty2 = ty1 = mem_copy_type(ty, area);
		ty = ty->bty;
		while (ty->cat == CAT_ARRAY) {
			ty1->bty = mem_copy_type(ty, area);
			ty1 = ty1->bty;
			ty = ty->bty;
		}
		ty1->bty = qualify(ty, is_const, is_volatile, area);
		return ty2;
	}
	/* type is pointer, struct or union,
	then we must clone the outmost type itme of type-list*/
	ty1 = mem_copy_type(ty, area);
	ty1->is_const = is_const;
	ty1->is_volatile = is_volatile ? 1 : 0;

	return ty1;
}

/*===------------------------------------------------------------------------------

-------------------------------------------------------------------------------===*/
void enter_scope() {
	level++;

	return;
}

/*===-------------------------------------------------------------------------------

--------------------------------------------------------------------------------===*/
static void clean(sym_tab **st, int lev) {
	sym_tab *t = *st;

	assert(t->scope <= lev);
	if (t->scope == lev)
		*st = t->prev;
	//for(t = *st; t->scope == lev; t = t->prev)
		//*st = t->prev;

	return;
} 

/*===-------------------------------------------------------------------------------

--------------------------------------------------------------------------------===*/
void exit_scope() {
	//rm_types();
	clean(&ids_tb, level);
	clean(&tags_tb, level);
	level--;

	return;
}

/*===-------------------------------------------------------------------------------

--------------------------------------------------------------------------------===*/
type *pointer(type *ref, int area) {
	if (area <= 0)
		area = level > GLOBAL ? TY : PERM;
	return new_type(CAT_POINTER, ref, 4, 4, area);
}

/*===-------------------------------------------------------------------------------

--------------------------------------------------------------------------------===*/
type *new_array(type *base, int len, int area) {
	if (base->size == 0)
		error(-1, zcc_line_no, "can not creat array whose element's size is 0\n");
	
	return new_type(CAT_ARRAY, base, base->size * len, base->size, area);
}
