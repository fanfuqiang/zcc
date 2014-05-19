/**
Copyright (C) 2013
	Writed by zet
*/

#include"stdhdr.h"
#include"zcchdr.h"

/* definition of name spaces*/
static sym_tab
	ns_labs = {LOCAL, },		/* labels*/
	ns_tags = {GLOBAL, },		/* tags*/
	ns_ids = {GLOBAL, },		/* identifiers*/
	//ns_types = {GLOBAL, },
	ns_cnsts = {CONSTANT, }		/* constants, but enumrator constant exist int ns_ids table*/
;
/* gloabl definetions*/
sym_tab *globals_tb = &ns_ids;
sym_tab *ids_tb = &ns_ids;
sym_tab *cnsts_tb = &ns_cnsts;
sym_tab *tags_tb = &ns_tags;
sym_tab *labels_tb = &ns_labs;

/* for generated name*/
static int _num_temps = 0;
//static int _num_statics = 0;
//static int _zcc_init_symtab;

/*===--------------------------------------------------------------------------
find the symbol, it's scope >= lev
----------------------------------------------------------------------------===*/
symbol *look_up(sym_tab *st, char *name) {
	entry *p;
	int i = str2key(name, strlen(name), H_SIZE);

	assert(st);
	for (; st; st = st->prev) {
		p = st->buckets[i];
		for (; p; p = p->next)
			if (p->sym.name == name)
				return &p->sym;
			else
				assert(strcmp(p->sym.name, name) && "Panic, string not unique");
	}

	return NULL;
}

/*===--------------------------------------------------------------------------
find the symbol, it's scope >= lev
----------------------------------------------------------------------------===*/
symbol *look_up_level(sym_tab *st, char *name, int lev) {
	entry *p;
	int i = str2key(name, strlen(name), H_SIZE);
	if (lev > st->scope)
		return NULL;
	/* lev <= st->scope*/
	for (; st && st->scope != lev; st = st->prev)
		;
	/* after GLOBAL is LOCAL, the value of level is continuous*/
	assert(st && st->scope == lev);
	for (p = st->buckets[i]; p; p = p->next)
		if (p->sym.name == name)
			return &p->sym;
		else
			assert(strcmp(p->sym.name, name) && "Panic, string not unique");

	return NULL;
}

/*===--------------------------------------------------------------------------
need add a type to parameter?
---------------------------------------------------------------------------===*/
symbol *install(sym_tab **st, char *name, int lev, int area) {
	sym_tab **pps = st;
	sym_tab *ps, *tp;
	//symbol sym;
	int index = str2key(name, strlen(name), H_SIZE);
	entry *pe;

	if (area <= 0)
		area = lev > GLOBAL ? FUNC :PERM; 
	//when symbol table has be initialized, they should point to some one 
	assert(*st);
	assert(lev >= (*st)->scope);
	//NEW0(ps, PERM);
	// find the proper symbol table
	for (; *pps; pps = &(*pps)->prev)
		if ((*pps)->scope <= lev)
			break;
	if ((*pps)->scope == lev)
		tp = *pps;
	/*level less than GLOBAL is impossible*/
	else {
		/*dynamic allocated symbol tabel must be FUNC, globals_tb had beedn statically allocated*/
		NEW0(ps, FUNC);
		ps->scope = lev;
		ps->prev = *pps;
		//(*pps)->prev = ps;
		*st = tp = ps;
	}

	NEW0(pe, area);
	pe->next = tp->buckets[index];
	tp->buckets[index] = pe;
	pe->sym.name = name;
	pe->sym.scope = lev;
	/* specifial for label*/
	if (*st == labels_tb)
		pe->sym.asm_name = gen_tmp_name("LA");
	
	return &pe->sym;
}

/*===--------------------------------------------------------------------------
T - temp
S - struct/union/enum tag
L - label
--------------------------------------------------------------------------===*/
char *gen_tmp_name(char *s) {
	// sprintf will write the last 0 into buffer
	sprintf(buffer_128, "%s.%05d", s, _num_temps++);

	return simple_string(buffer_128);
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
symbol *gen_label(char *s) {
	symbol *p;

	NEW0(p, FUNC);
	if (s == NULL)
		s = "LA";
	p->name = p->asm_name = gen_tmp_name(s);

	return p;
}

/*===---------------------------------------------------------------------------

----------------------------------------------------------------------------===*/
symbol *constant(value v, type *ty) {
	entry *p;
	int index;
	switch (ty->cat) {
	case CAT_CHAR:
	case CAT_UCHAR:
	case CAT_SHORT:
	case CAT_USHORT:
	case CAT_INT:
	case CAT_UINT:
	case CAT_POINTER:
	case CAT_ARRAY:
		index = (v.i + (v.i >> 7)/* 7 = log(H_SZIE)*/) & H_SIZE;
		for (p = cnsts_tb->buckets[index]; p; p = p->next) {
			if (p->sym.u.cnst.i == v.i)
				return &p->sym;
		}
		break;
	case CAT_FLOAT:
		index = (int)v.f;
		index = (index + (index >> 7)) & H_SIZE;
		for (p = cnsts_tb->buckets[index]; p; p = p->next) {
			if (p->sym.u.cnst.f - v.f < FLT_EPSILON
				|| v.f - p->sym.u.cnst.f < FLT_EPSILON)
				return &p->sym;
		}
		break;
	case CAT_DOUBLE:
		index = (int)v.d;
		index = (index + (index >> 7)) & H_SIZE;
		for (p = cnsts_tb->buckets[index]; p; p = p->next) {
			if (p->sym.u.cnst.d - v.d < DBL_EPSILON
				|| v.d - p->sym.u.cnst.d < DBL_EPSILON)
				return &p->sym;
		}
		break;
	default:
		assert(0);
	}
	/* make a new one*/
	NEW0(p, PERM);
	p->sym.scope = CONSTANT;
	p->sym.ty = ty;
	p->sym.name = gen_tmp_name("LC");
	p->sym.u.cnst = v;
	p->sym.is_immediate = 1;
	/* add to constant table*/
	p->next = cnsts_tb->buckets[index];
	cnsts_tb->buckets[index] = p;

	return &p->sym;
}

/*===--------------------------------------------------------------------------
generate a symbol but do not install any symbol table
---------------------------------------------------------------------------===*/
symbol *gen_symbol(char *name, type *ty, int area) {
	symbol *sym;

	NEW0(sym, area);
	sym->name = name;
	sym->ty = ty;

	return sym;
}
