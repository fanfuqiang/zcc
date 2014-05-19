/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

static void enrol_loc(symbol *loc, symbol *s);
/* register table definition*/
symbol i32_regs[] = {
	{"eax", },
	{"ebx", },
	{"ecx", },
	{"edx", },
	{"esi", },
	{"edi", },
	{"ebp", },
	{"esp", }
};

symbol f80_regs[] = {
	{"st(0)", },
	{"st(1)", }
};

/* LRU for register*/
static dlist *lru_queue;
static slist *free_i32_locs;
static slist *free_f64_locs;

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
void init_lru_queue() {
	int i = 0;
	for (; i < EBP; i++)
		add2dlist_tail(&lru_queue, &i32_regs[i], FUNC);
	
	return;
}

/*===-------------------------------------------------------------------------
add new item in a higher address
-------------------------------------------------------------------------===*/
static void lru_add(symbol *s) {
	dlist *p = lru_queue;
	
	/* already is head, nothing to do*/
	if (p->it == s)
		return;
	do {
		if (p->it == s) {
			/* delete*/
			p->next->prev = p->prev;
			p->prev->next = p->next;
			/* add to head*/
			p->next = lru_queue;
			p->prev = lru_queue->prev;
			lru_queue->prev->next = p;
			lru_queue->prev = p;
			lru_queue = p;
		}
		p = p->next;
	} while (p != lru_queue);

	assert(0);
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static symbol *creat_spill_loc(type *ty) {
	symbol *p;
	int *off = &_F.cur_ebp_offset;

	assert(ty);
	assert(is_scalar_cat(ty->cat));
	NEW0(p, FUNC);
	p->scope = LOCAL;
	p->ty = ty;
	p->is_free = 1;
	_align(off, ty->size);
	p->ebp_off = *off;		/* spill locatation*/
	*off += ty->size;

	return p;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void free_spill_loc(symbol *s) {
	s->is_free = 1;
	s->u.reg.alias = NULL;
	if (s->ty == int_type)
		add2slist_head(&free_i32_locs, s, FUNC);
	else if (s->ty == double_type)
		add2slist_head(&free_f64_locs, s, FUNC);
	else
		assert(0);

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
symbol *get_spill_loc(int kind) {
	symbol *s;

	switch (kind) {
	case i8:
	case i32:
	case f32:
		if (free_i32_locs) {
			s = (symbol *)get_slist_head(&free_i32_locs);
			assert(s->is_free == 1);
		} else
			s = creat_spill_loc(int_type);
		break;
	case f64:
		if (free_f64_locs) {
			s = (symbol *)get_slist_head(&free_f64_locs);
			assert(s->is_free == 1);
		} else
			s = creat_spill_loc(double_type);
		break;
	default:
		assert(0);
	}

	return s;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
void spill(symbol *r) {
	symbol *s, *loc;
	dlist *d = r->u.reg.alias;
	int flag_alter;

	if (r->is_free)
		return;
	assert(d);
	do {
		s = (symbol *)d->it;
		if (flag_alter = s->is_ignore_alter)
			continue;
		s->reg = NULL;	//must be here
		if (s->is_itmp) {
			loc = get_spill_loc(eval_kind(r->ty->cat));
			gas_reg_to_mem(loc, s);
			//s->is_split = 1;
			enrol_loc(loc, s);
		} else if (s->is_need_wb) {
			gas_reg_to_mem(s, s);
			//relieve_reg(s->reg, s);
		}
		s->is_in_reg = 0;
		s->is_need_wb = 0;
	} while ((d = d->next) != r->u.reg.alias);
	// ignore alter() call sipll
	if (!flag_alter) {
		r->is_free = 1;
		r->u.reg.alias = NULL;
	}
	
	return;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static symbol *select_spill(int last) {
	symbol *r;
	dlist *d = lru_queue->prev;		// least recently used

	while ((r = (symbol *)d->it))
		if (r < &i32_regs[last])
			return r;
		else
			d = d->prev;
	//assert(0);
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
static symbol *get_reg_int(int last) {
	int i = 0;
	symbol *r;

	/* has free register*/
	for (; i < last; i++) {
		r = &i32_regs[i];
		if (r->is_free) {
			r->is_free = 0;
			return r;
		}
	}
	/* spill one register*/
	r = select_spill(last);
	spill(r);
	assert(r->is_free == 1);
	lru_add(r);

	return r;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
symbol *get_reg_i8() {
	symbol *r = get_reg_int(ESI);
	r->is_i8 = 1;

	return r;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
symbol *get_reg(int kind) {
	symbol *r = NULL;

	switch (kind) {
	case i8:
		r = get_reg_i8();
		break;
	case i32:
		r =  get_reg_int(EBP);
		break;
	case f32:
	case f64:
		assert(0);
	}
	assert(r);
	r->u.reg.alias = NULL;

	return r;
}

/*===-------------------------------------------------------------------------
s is a register or location(float/double), spill its alias
---------------------------------------------------------------------------===*/
void alter(symbol *s) {
	symbol *reg = s->reg;

	/* memory and has no register*/
	if (s->is_itmp == 0 && s->is_in_reg == 0)
		return;
	assert(reg);
	s->is_ignore_alter = 1;
	spill(reg);
	//enrol_reg(reg, s);

	return;
}

/*===-------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
void init_regs() {
	int i = 0;
	symbol *p;
	
	/* esp and ebp is reserved*/
	for (p = i32_regs; i < EBP; i++, p++) {
		p->is_free = 1;
		p->scope = GLOBAL;
		p->ty = int_type;
	}
	i32_regs[EBP].scope = GLOBAL;
	i32_regs[EBP].ty = int_type;
	i32_regs[ESP].scope = GLOBAL;
	i32_regs[ESP].ty = int_type;
	f80_regs[0].is_free = 1;
	f80_regs[0].scope = GLOBAL;
	f80_regs[0].ty = double_type;
	f80_regs[1].is_free = 1;
	f80_regs[1].scope = GLOBAL;
	f80_regs[1].ty = double_type;
	/* initialize the overlay part*/
	i32_regs[EAX].name_i8_low = "al";
	//(&i32_regs[EAX])->name_r8_high = "ah";
	i32_regs[EBX].name_i8_low = "bl";
	//(&i32_regs[EBX])->name_r8_high = "bh";
	i32_regs[ECX].name_i8_low = "cl";
	//(&i32_regs[ECX])->name_r8_high = "ch";
	i32_regs[EDX].name_i8_low = "dl";
	//(&i32_regs[EDX])->name_r8_high = "dh";
	init_lru_queue();

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void enrol_loc(symbol *loc, symbol *s) {
	/* memory operand never need spill to a stack location*/
	assert(s->is_itmp);
	add2dlist_head(&loc->u.reg.alias, s, FUNC);
	s->reg = loc;
	s->is_split = 1;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void relieve_loc(symbol *reg, symbol *s) {
	dlist **dd = &reg->u.reg.alias;

	assert(*dd);
	dlist_delete_item(dd, s);
	if (*dd == NULL) {
		reg->is_free = 1;
		//reg->is_i8 = 0;
		free_spill_loc(reg);
	}
	s->reg = NULL;
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void enrol_reg(symbol *reg, symbol *s) {
	add2dlist_head(&reg->u.reg.alias, s, FUNC);
	s->reg = reg;
	if (s->is_itmp)
		s->is_split = 0;
	else
		s->is_in_reg = 1;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void relieve_reg(symbol *reg, symbol *s) {
	dlist **dd = &reg->u.reg.alias;

	assert(*dd);
	dlist_delete_item(dd, s);
	if (*dd == NULL) {
		reg->is_free = 1;
		reg->is_i8 = 0;
	}
	s->reg = NULL;
	
	return;
}

/*===-------------------------------------------------------------------------
make sure s in register
---------------------------------------------------------------------------===*/
void alloc_reg(symbol *s) {
	symbol *reg;

	if (s->is_itmp) {
		assert(s->is_need_st == 0);
		/* never has register*/
		if (s->is_split && s->reg == NULL) {
			reg = get_reg(eval_kind(s->ty->cat));
			reg->is_free = 0;
			enrol_reg(reg, s);
		/* had a register but already split*/
		} else if (s->is_split && s->reg)
			gas_restore_loc(s);
		else if (s->is_split == 0 && s->reg)
			;	// has register now
		else
			assert(0);
	} else {
		/* has no register*/
		if (s->is_in_reg == 0 && s->reg == NULL) {
			reg = get_reg(eval_kind(s->ty->cat));
			gas_mem_to_reg(reg, s);
			reg->is_free = 0;
			enrol_reg(reg, s);
		} else if (s->is_in_reg && s->reg)
			;	// has register now
		else
			assert(0);
	}

	return;
}

