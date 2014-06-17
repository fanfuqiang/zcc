/**
Copyright (C) 2013
	Writed by zet
*/

#include "zcchdr.h"
#include "stdhdr.h"
/// used by inst.h
static void gas_prolog(icode *ic);
static void gas_branch(icode *ic);
static void gas_label(icode *ic);
static void gas_cmp(icode *ic);
static void gas_mov(icode *ic);
static void gas_mov_block(icode *ic);
static void gas_mov_arg(icode *ic);
static void gas_cvt(icode *ic);
static void gas_addr(icode *ic);
static void gas_bin(icode *ic);
static void gas_inc(icode *ic);
static void gas_unary(icode *ic);
static void gas_epilog(icode *ic);
static char *gas_op(symbol *s);
/* GNU as print function table*/
static void (*gas_print[])(icode *) = {
#define inst(a, b, c, d) d,
#define inst_last(a, b, c, d) d
#include "inst.h"
#undef inst_last
#undef inst
};

#define post_mask 0x1f
#define prefix_char '_'
#define TEXT_BUF_SIZE 4096
#define DATA_BUF_SIZE 2048
/* GNU as instruction table*/
char *gas_inst[] = {
#define inst(a, b, c, d) c,
#define inst_last(a, b, c, d) c
#include "inst.h"
#undef inst_last
#undef inst
};
///
static zbuf zcc_buf_text;
static zbuf *buf_text = &zcc_buf_text;
static zbuf zcc_buf_data;
static zbuf *buf_data = &zcc_buf_data;
static zbuf zcc_buf_bss;
static zbuf *buf_bss = &zcc_buf_bss;
static zbuf zcc_buf_rodata;
static zbuf *buf_rodata = &zcc_buf_rodata;
//static FILE *f = file_text;
#define is_fd_cat(c) (c == CAT_FLOAT || c == CAT_DOUBLE)
#define st0 (&f80_regs[0])
#define st1 (&f80_regs[1])
//#define zpt(f, ...) (zbuf_print(buf_text, f, ...))

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void zpt(char *format, ...) {
	va_list arg;
	va_start(arg, format);
	zbuf_vprint(buf_text, format, arg);
	va_end(arg);
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void zpd(char *format, ...) {	
	va_list arg;
	va_start(arg, format);
	zbuf_vprint(buf_data, format, arg);
	va_end(arg);
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void zpr(char *format, ...) {
	va_list arg;
	va_start(arg, format);
	zbuf_vprint(buf_rodata, format, arg);
	va_end(arg);
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void zps(char *inst, char *op1, char *op2) {
	if (op1 && op2)
		zpt("\t%s\t%s, %s\n", op1, op2);
	else if (op1 && op2 == NULL)
		zpt("\t%s\t%s\n", op1);
	else if (op1 == NULL && op2)
		zpt("\t%s\t%s\n", op2);
	else
		zpt("\t%s\n");

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static char *gas_postfix(int post) {
	switch (post) {
	case i8:
		return "b";
	case i16:
		return "w";
	case i32:
	case f64:
		return "l";
	case f32:
		return "s";
	default:
		assert(0);
	}
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static char *gas_get_inst(int node) {
	int index, post, size = 0;

	assert(node);
	index = node >> 5;
	post = node & 0x1f;
	assert(gas_inst[index]);
	sprintf(buffer_128, "%s", gas_inst[index]);
	//sprintf(buffer_128 + size, "%s", gas_postfix(post));
	strcat(buffer_128, gas_postfix(post));
	
	return buffer_128;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void gas_restore_loc(symbol *s) {
	symbol *s1, *reg, *loc = s->reg;
	dlist *d;

	assert(s->is_itmp && s->is_split);
	reg = get_reg(eval_kind(s->ty->cat));
	d = loc->u.reg.alias;
	do {
		s1 = (symbol *)d->it;
		assert(s->is_split);
		s1->is_split = 0;
		s1->reg = reg;
		d = d->next;
	} while (d != loc->u.reg.alias);
	reg->u.reg.alias = loc->u.reg.alias;
	free_spill_loc(loc);
	// print instruction
	gas_mem_to_reg(reg, loc);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static char *gas_op_f64(symbol *s, int offset) {
	int cat = s->ty->cat;

	assert(cat == CAT_DOUBLE);
	if (s->is_itmp) {
		assert(s->is_split);
		return gas_op_f64(s->reg, offset);
	} else {
		assert(s->ebp_off);
		sprintf(buffer_128, "-%%d(%%ebp)", s->ebp_off - offset);
		return buffer_128;
	}
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void assign_st(symbol *s) {
	int cat = s->ty->cat;
	
	// must be float and double
	assert(is_fd_cat(cat));
	//assert(s->is_itmp && s->is_split);
	if (s->is_immediate) {
		if (cat == CAT_FLOAT) {
			zpt("\tmovl\t%d, %s", s->u.cnst.i, gas_op(&_F.spill_f32));
			s = &_F.spill_f32;
		} else {
			zpt("\tmovl\t%d, %s", s->u.cnst.i, gas_op_f64(&_F.spill_f64, 0));
			zpt("\tmovl\t%d, %s", s->u.cnst.i, gas_op_f64(&_F.spill_f64, 4));
			s = &_F.spill_f64;
		}
	} else {
		if (cat == CAT_FLOAT)
			zpt("\tflds\t%s\n", gas_op(s));
		else
			zpt("\tfldl\t%s\n", gas_op(s));
	}
	/* free*/
	if (s->reg && s->use_times <= 1)
		free_spill_loc(s->reg);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void spill_st(symbol *s) {
	symbol *loc;
	assert(is_fd_cat(s->ty->cat));
	//assert(s->is_itmp);
	if (s->is_itmp) {
		loc = get_spill_loc(eval_kind(s->ty->cat));
		s->is_split = 1;
		s->reg = loc;
	} else
		loc = s;
	if (s->ty->cat == CAT_FLOAT)
		zpt("\tfstps\t%s\n", gas_op(loc));
	else
		zpt("\tfstpl\t%s\n", gas_op(loc));

	return;
}

/*===-------------------------------------------------------------------------
allocate register for operand
	itmp(alloc reg) x itmp(alloc reg)
	itmp(alloc reg) x mem_op
	itmp(alloc reg) x imm
	mem_op x itmp(alloc reg)
	mem_op x mem_op(alloc and mov to register)
	mem_op x imm
	imm x ...	-	illegal
---------------------------------------------------------------------------===*/
static void legalize_ops(icode *ic) {
	symbol *s, *op[2] = {ic->left, ic->right};
	int i = 0;

	if ((ic->node & ~post_mask) == IC_MOV)
		return;
	if (op[0])
		assert(op[0]->is_immediate == 0);
	// itmp must has register
	while (i < 2) {
		s = op[i];
		/* make sure itmp which is not float type in register*/
		if (s && s->is_itmp && s->is_need_st == 0)
			alloc_reg(s);
		if (i++ == 1)
			return;
	}
#if 0
	// two memory operand must has register
	if (s1->is_itmp == 0 && s2->is_itmp == 0
		&& s1->is_in_reg == 0 && s2->is_in_reg == 0) {
			// let st0 be the result operand
			if (s2->is_need_st == 0 && s2->use_times > 1)
				alloc_reg(s2);
			else
				alloc_reg(s1);
	}
#endif

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
int eval_kind(int cat) {
	switch (cat) {
	case CAT_CHAR:
	case CAT_UCHAR:
		return i8;
	case CAT_INT:
	case CAT_UINT:
	case CAT_POINTER:
		return i32;
	case CAT_FLOAT:
		return f32;
	case CAT_DOUBLE:
		return f64;
	default:
		assert(0);
	}
}

/*===-------------------------------------------------------------------------
	GAS_IMM = 1,		// s->immediate == 1
	GAS_VAR,			
	GAS_STR,			// s->immediate == 1
	GAS_ITMP,			// s->is_itmp == 1
	GAS_REG,			// s->is_in_reg == 1
---------------------------------------------------------------------------===*/
static char *gas_op(symbol *s) {
	int cat = s->ty->cat;
	
	if (s->is_immediate) {
		if (cat == CAT_CHAR)
			sprintf(buffer_128, "$%d", 0xff & s->u.cnst.i);
		else
			sprintf(buffer_128, "$%d", s->u.cnst.i);
		return buffer_128;
	} else if (s->is_itmp) {
		/** virtual register*/
		assert(s->reg);
		// float
		if (is_fd_cat(cat)) {
			/* when begin output every icode, fpu register stack must be clear
			after every icode has be output, fpu register must be spilt
			this is sucks, next version will be rewrite the whole backend*/
			assert(s->is_split);
			return gas_op(s->reg);
		}
		// integer
		if (s->reg->is_i8)
			sprintf(buffer_128, "%%%s", s->reg->name_i8_low);
			//return s->reg->name_i8_low;
		else if (s->is_addr)		// indirect access
			sprintf(buffer_128, "(%%%s)", s->reg->name);
			//return buffer_128;
		else
			sprintf(buffer_128, "%%%s", s->reg->name);
		return buffer_128;
	} else {
		/** variable*/
		if (s->is_in_reg) {		//in register
			return s->reg->name;
		} else if (s->scope < LOCAL || s->is_external)	//.data
			return s->name;
		else if (s->is_static)		//.data, .bss
			return s->asm_name;
		else {						//stack area
			assert(s->ebp_off);
			sprintf(buffer_128, "-%d(%%ebp)", s->ebp_off);
			return buffer_128;
		}
	}

	assert(0);
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void print_inst(int node, symbol *s1, symbol *s2) {
	zpt("\t%s\t", gas_get_inst(node));
	if (s2) {
		assert(s1);
		zpt("%s", gas_op(s2));
		zpt(", %s\n", gas_op(s1));
	} else
		zpt("%s\n", gas_op(s1));

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_mov(icode *ic) {
	symbol *s1, *s2;
	int cat;

	s1 = ic->left;
	s2 = ic->right;
	assert(s1 && s2);
	cat = s1->ty->cat;
	// should make sure this ???
	assert(cat == s2->ty->cat);
	/* float and double*/
	if (is_fd_cat(cat)) {
		assign_st(s2);
		spill_st(s1);
	}
	/* intergal*/
	s1->is_need_wb = 1;
	// s1 is immediate value
	if (s1->is_immediate)
		assert(0);
	// s1 is virtual register
	else if (s1->is_itmp) {
		if (s2->is_immediate) {
			// itmp x imm
			alloc_reg(s1);
			print_inst(ic->node, s1, s2);
		} else if (s2->is_itmp) {
			// itmp x itmp
			alloc_reg(s2);
			enrol_reg(s2->reg, s1);
		} else {
			// itmp x mem
			if (s2->is_in_reg)
				enrol_reg(s2->reg, s1);
			else {
				alloc_reg(s1);
				print_inst(ic->node, s1, s2);
			}
		}
	// s1 is a memory location
	} else {
		if (s2->is_immediate)
			// mem x imm
			;
		else if (s2->is_itmp)
			// mem x itmp
			alloc_reg(s2);
		else {
			// mem x mem
			if (s1->is_in_reg == 0 && s2->is_in_reg == 0)
				if (s2->use_times == 1)
					alloc_reg(s1);
				else
					alloc_reg(s2);
		}
		print_inst(ic->node, s1, s2);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_mov_block(icode *ic) {
	symbol *s1, *s2, *reg;
	int size;
	icode *ic1;

	s1 = ic->left;
	s2 = ic->right;
	assert(s1 && s2);
	size = s1->ty->size;
	NEW0(ic1, FUNC);
	// leal s2, esi
	reg = &i32_regs[ESI];
	spill(reg);
	ic1->left = reg;
	ic1->right = s2;
	gas_addr(ic1);
	// leal s1, edi
	reg = &i32_regs[EDI];
	spill(reg);
	ic1->left = reg;
	ic1->right = s2;
	gas_addr(ic1);
	//movl size, ecx
	//rep movsb
	reg = &i32_regs[ECX];
	spill(reg);
	zpt("\tmovl\t$%d, %s", size, "ecx");
	zpt("\trep\tmovsb\n");
	/* free register manually*/
	i32_regs[ESI].is_free = 1;
	i32_regs[EDI].is_free = 1;
	i32_regs[ECX].is_free = 1;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_mov_arg(icode *ic) {
	symbol *s1 = ic->left;
	int offset = ic->u.arg_off, size;

	size = s1->ty->size;
	if (size == 4)
		zpt("\tmovl\t%s, -%d(%%ebp)", gas_op(s1), offset);
	else if (size == 8) {
		zpt("\tmovl\t%s, -%d(%%ebp)", gas_op(s1), offset);
		zpt("\tmovl\t%s, -%d(%%ebp)", gas_op(s1), offset + 4);
	} else
		assert(0);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_cmp(icode *ic) {
	symbol *s1, *s2;
	int cat, k, post = ic->node & post_mask;

	s1 = ic->left;
	s2 = ic->right;
	cat = s1->ty->cat;
	k = ic->node >> 5;
	switch (k) {
	case IC_CMP:
		if (is_fd_cat(cat)) {
			assign_st(s2);
			assign_st(s1);
			zps("fcompp", NULL, NULL);
			zps("fstsw", NULL, NULL);
			zps("sahf", NULL, NULL);
		} else
			print_inst(ic->node, s1, s2);
		break;
	case IC_CMP_0:
	case IC_CMP_1:
		if (is_fd_cat(cat)) {
			if (k == IC_CMP_0)
				zps("fldz", NULL, NULL);
			else
				zps("fld1", NULL, NULL);
			assign_st(s1);
			zps("fcompp", NULL, NULL);
			zps("fstsw", NULL, NULL);
			zps("sahf", NULL, NULL);
		} else {
			if (k == IC_CMP_0)
				print_inst(ic->node, s1, cnst_zero);
			else
				print_inst(ic->node, s1, cnst_one);
		}
		break;
	default:
		assert(0);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_inc(icode *ic) {
	int kind, cat;
	symbol *s1 = ic->left;
	char *op;
	assert(s1 && ic->right == 0);

	kind = ic->node >> 5;
	cat = s1->ty->cat;
	op = gas_op(s1);
	if (is_fd_cat(cat)) {
		zpt("\tfld1\n");	//mov st0, 1
		if (cat == CAT_FLOAT) {
			if (kind == IC_INC)
				zpt("\tfadds\t%s\n", op);
			else
				zpt("\tfsubs\t%s\n", op);
			zpt("\tfstps\t%s\n", op);		//st0 must be free immediately
		} else {
			if (kind == IC_INC)
				zpt("\tfaddl\t%s\n", op);
			else
				zpt("\tfsubl\t%s\n", op);
			zpt("\tfstpl\t%s\n", op);
		}
	} else
		zpt("\t%s\t%s\n", gas_get_inst(ic->node), op);

	return;
}

/*===-------------------------------------------------------------------------
mov mem, reg
---------------------------------------------------------------------------===*/
void gas_mem_to_reg(symbol *reg, symbol *s) {
	icode *ic;
	int post = gen_post(s->ty->cat, s->ty->cat);
	
	assert(reg->ty->cat = s->ty->cat);
	assert(s->is_itmp == 0);
	NEW0(ic, FUNC);
	ic->node = IC_MOV + post;
	ic->left = reg;
	ic->right = s;
	gas_mov(ic);

	return;
}

/*===-------------------------------------------------------------------------
mov reg, mem - write back to memory
-------------------------------------------------------------------------===*/
void gas_reg_to_mem(symbol *s, symbol *reg) {
	icode *ic;
	int post = gen_post(s->ty->cat, s->ty->cat);

	assert(s->is_itmp == 0);
	NEW0(ic, FUNC);
	ic->node = IC_MOV + post;
	ic->left = s;
	ic->right = reg;
	gas_mov(ic);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_cvt(icode *ic) {
	symbol *s1, *s2;
	//char *insn = gas_get_inst(ic);
	int post = ic->node & post_mask;
	int k1, k2;
	s1 = ic->left;
	s2 = ic->right;
	assert(s1->is_itmp);
	k1 = eval_kind(s1->ty->cat);
	k2 = eval_kind(s2->ty->cat);
	if (s1->reg == NULL)
		s1->reg = get_reg(k1);
	switch (post) {
	case i8_i32:
		assert(k1 == i8 && k2 == i32);
		print_inst(IC_MOV + i8, s1, s2);
		break;
	case i8_f32:
		assert(k1 == i8 && k2 == f32);
		assign_st(s2);
		zpt("\tfistps\t%s\n", gas_op(&_F.spill_f32));
		zpt("\tmovb\t%s, %s\n", gas_op(&_F.spill_f32), gas_op(s1));
		break;
	case i8_f64:
		assert(k1 == i8 && k2 == f64);
		assign_st(s2);
		zpt("\tfistpl\t%s\n", gas_op(&_F.spill_f64));
		zpt("\tmovb\t%s, %s\n", gas_op(&_F.spill_f64), gas_op(s1));
		break;
	case i32_i8:
		assert(k1 == i32 && k2 == i8);
		zpt("\tmovsbl\t%s, %s\n", gas_op(s2), gas_op(s1));
		break;
	case i32_f32:
		assert(k1 == i32 && k2 == f32);
		assign_st(s2);
		if (s1->is_itmp) {
			zpt("\tfistps\t%s\n", gas_op(&_F.spill_f32));
			zpt("\tmovl\t%s, %s\n", gas_op(&_F.spill_f32), gas_op(s1));
		} else
			zpt("\tfistps\t%s\n", gas_op(s1));
		break;
	case i32_f64:
		assert(k1 == i32 && k2 == f64);
		assign_st(s2);
		if (s1->is_itmp) {
			zpt("\tfistpl\t%s\n", gas_op(&_F.spill_f64));
			zpt("\tmovl\t%s, %s\n", gas_op(&_F.spill_f64), gas_op(s1));
		} else
			zpt("\tfistpl\t%s\n", gas_op(s1));
		break;
	case f32_i8:
		assert(k1 == f32 && k2 == i8);
		zpt("\tmovsbl\t%s, %s\n", gas_op(s2), gas_op(&_F.spill_f32));
		zpt("\tfilds\t%s\n", gas_op(&_F.spill_f32));
		spill_st(s1);
		break;
	case f32_i32:
		assert(k1 == f32 && k2 == i32);
		if (s2->is_itmp) {
			zpt("\tmovl\t%s, %s\n", gas_op(s2), gas_op(&_F.spill_f32));
			zpt("\tfilds\t%s\n", gas_op(&_F.spill_f32));
		} else
			zpt("\tfilds\t%s\n", gas_op(s2));
		spill_st(s1);
		break;
	case f32_f64:
		assert(k1 == f32 && k2 == f64);
		assign_st(s2);
		spill_st(s1);
		break;
	case f64_i8:
		assert(k1 == f64 && k2 == i8);
		zpt("\tmovsbl\t%s, %s\n", gas_op(s2), gas_op(&_F.spill_f32));
		zpt("\tfildl\t%s\n", gas_op(&_F.spill_f32));
		spill_st(s1);
		break;
	case f64_i32:
		assert(k1 == f64 && k2 == i32);
		if (s2->is_itmp) {
			zpt("\tmovl\t%s, %s\n", gas_op(s2), gas_op(&_F.spill_f32));
			zpt("\tfildl\t%s\n", gas_op(&_F.spill_f32));
		} else
			zpt("\tfildl\t%s\n", gas_op(s2));
		spill_st(s1);
		break;
	case f64_f32:
		assert(k1 == f64 && k2 == f32);
		assign_st(s2);
		spill_st(s1);
		break;
	default:
		assert(0);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void alloc_reg_certain(symbol *s, symbol *reg) {

	if (s->reg == reg)
		return;
	reg->is_free = 0;
	if (s->is_itmp) {
		assert(s->is_need_st == 0);
		/* never has register*/
		if (s->is_split && s->reg == NULL)
			enrol_reg(reg, s);
		/* had a register but already split*/
		else if (s->is_split && s->reg) {
			zps("movl", gas_op(s), reg->name);
			relieve_loc(s->reg, s);
			enrol_reg(reg, s);
		} else if (s->is_split == 0 && s->reg) {
		// has register now
			zps("movl", gas_op(s), reg->name);
			relieve_reg(s->reg, s);
			enrol_reg(reg, s);
		} else
			assert(0);
	} else {
		/* has no register*/
		if (s->is_in_reg == 0 && s->reg == NULL) {
			zps("movl", gas_op(s), reg->name);
			enrol_reg(reg, s);
		} else if (s->is_in_reg && s->reg) {
			zps("movl", gas_op(s), reg->name);
			relieve_reg(s->reg, s);
			enrol_reg(reg, s);	// has register now
		} else
			assert(0);
	}

	return;


}

/*===-------------------------------------------------------------------------
generate code for icode of binary expreesion
---------------------------------------------------------------------------===*/
static void gas_bin(icode *ic) {
	symbol *reg1, *reg2, *s1, *s2;
	int k, cat, post = ic->node & post_mask;

	s1 = ic->left;
	s2 = ic->right;
	assert(s1->ty->cat == s2->ty->cat);
	cat = s1->ty->cat;
	k = ic->node & ~post_mask;
	s1->is_need_wb = 1;
	switch (k) {
	case IC_DIV:
		if (is_fd_cat(cat)) {
			assign_st(s2);
			assign_st(s1);
			zps("fdivp", NULL, NULL);
			spill_st(s1);
		} else {
			reg1 = &i32_regs[EAX];
			spill(reg1);
			alloc_reg_certain(s1, reg1);
			reg2 = &i32_regs[EDX];
			spill(reg2);
			zps("cdq", NULL, NULL);
			zps("idivl", gas_op(s2), NULL);
			reg2->is_free = 1;
		}
		return;
	case IC_MUL:
		if (is_fd_cat(cat)) {
			assign_st(s2);
			assign_st(s1);
			zps("fmulp", NULL, NULL);
			spill_st(s1);
		} else {
			reg1 = &i32_regs[EAX];
			spill(reg1);
			alloc_reg_certain(s1, reg1);
			reg2 = &i32_regs[EDX];
			spill(reg2);
			zps("imull", gas_op(s2), NULL);
			reg2->is_free = 1;
		}
		return;
	case IC_MOD:
		break;
	case IC_ADD:
		if (is_fd_cat(cat)) {
			assign_st(s2);
			assign_st(s1);
			zps("faddp", NULL, NULL);
			spill_st(s1);
		}
		break;
	case IC_SUB:
		if (is_fd_cat(cat)) {
			assign_st(s2);
			assign_st(s1);
			zps("fsubp", NULL, NULL);
			spill_st(s1);
		}
		break;
	case IC_LS:
	case IC_RS:
	case IC_BAND:
	case IC_BXOR:
	case IC_BIOR:
	default:
		assert(0);
	}
	print_inst(ic->node, s1, s2);
	s1->is_need_wb = 0;
	alter(s1);
	s1->is_need_wb = 1;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_label(icode *ic) {
	zpt("%s:\n", ic->left->name);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_addr(icode *ic) {
	symbol *s1, *s2;

	s1 = ic->left;
	s2 = ic->right;
	if (s2->scope < LOCAL || s2->is_external)	//global
		zpt("\tmovl\t$%s, %s\n", s2->name, gas_op(s1));
	else if (s2->is_static)
		zpt("\tmovl\t$%s, %s\n", s2->asm_name, gas_op(s1));
	else	//local
		zpt("\tleal\t-%d(%%ebp), %s\n", s2->ebp_off, gas_op(s1));

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_branch(icode *ic) {
	symbol *s1 = ic->left;
	char *inst = gas_get_inst(ic->node);
	if (s1->is_addr)		// indirect branch
		zpt("\t%s\t*%s\n", inst, gas_op(s1));
	else
		zpt("\t%s\t%s\n", inst, gas_op(s1));
	if (ic->node >> 5 == IC_CALL) {
		spill(&i32_regs[EAX]);
		spill(&i32_regs[ECX]);
		spill(&i32_regs[EDX]);
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_unary(icode *ic) {
	assert(ic->left && ic->right == NULL);
	print_inst(ic->node, ic->left, NULL);
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static int is_has_reg(symbol *s) {
	// virtual register
	if (s->is_itmp && s->is_split == 0) {	//zcc make sure itmp has rigister when generate code 
		assert(s->reg);
		return 1;
	}
	// not zcc created identifiers
	if (s->is_in_reg) {
		assert(s->reg);
		return 1;
	}

	return 0;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void try_free_reg(icode *ic) {
	symbol *s1, *s2;
	s1 = ic->left;
	s2 = ic->right;

	if (s1 && --s1->use_times == 0 && is_has_reg(s1))
		relieve_reg(s1->reg, s1);
	if (s2 && --s2->use_times == 0 && is_has_reg(s2))
		relieve_reg(s2->reg, s2);
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_ic(icode *head) {
	icode *p = head;

	while (p) {
		legalize_ops(p);
		gas_print[p->node >> 5](p);
		if (p->node == IC_FUNC_END)
			assert(p->next == NULL);
		try_free_reg(p);
		p = p->next;
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_text(icode *head) {
	zpt(".text\n");
	gas_ic(head);
	//fflush(f);
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_prolog(icode *ic) {
	int *p = &_F.cur_ebp_offset;

	_align(p, 16);
	zpt("%s:\n", _F.cur_func->name);
	zpt("\tpushl\t%%ebp\n");
	zpt("\tmovl\t%%esp, %%ebp\n");
	zpt("\tsubl\t$%d, %%esp\n", *p);
	zpt("\tpushl\t%%ebx\n");
	zpt("\tpushl\t%%esi\n");
	zpt("\tpushl\t%%edi\n");

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_epilog(icode *ic) {
	zpt("\tpopl\t%%edi\n");
	zpt("\tpopl\t%%esi\n");
	zpt("\tpopl\t%%ebx\n");
	zpt("\tleave\n");
	zpt("\tret\n");
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_rodata() {
	slist *d;
	symbol *s;

	if (_F.rodata_cnsts || _F.rodata_strings)
		zpr("\t.section\t.rodata\n");
	/**
	strings, constants
	float and double will convert to unsigned int and print
	*/
	for (d = _F.rodata_cnsts; d; d = d->next) {
		s = (symbol *)d->it;
		/* because same constant can appear more than once in source*/
		if (s->is_emitted == 0) {
			if (s->ty->size > 4) {		// double
				zpr("\t.align\t8\n");
				zpr("\t.long\t%d\n", *(int *)s->u.cnst.a);
				zpr("\t.long\t%d\n", *(int *)(s->u.cnst.a + 4));
			} else {					// int, float
				zpr("\t.align\t4\n");
				zpr("\t.long\t%d\n", s->u.cnst.i);
			}
			s->is_emitted = 1;
		}
	}
	// strings
	for (d = _F.rodata_strings; d; d = d->next) {
		s = (symbol *)d->it;
		/* because same constant can appear more than once in source*/
		if (s->is_emitted == 0) {
			assert (s->ty->cat == CAT_ARRAY);
			zpr("%s:\n", s->name);
			zpr("\t.ascii\t\"%s\\0\"\t", s->u.cnst.p);
			s->is_emitted = 1;
		}
	}
	//fflush(f);
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_izer(symbol *s) {
	ast *t;

	t = s->iz;
	assert(t);
	/* 
	.globl name
	.align N
	name:
	*/
	if (s->is_emitted)
		return;
	if (s->is_external) {
		zpd("\t.globl\t%s\n", s->name);
		if (s->ty->align > 1)
			zpd("\t.align\t%d\n", s->ty->align);
		zpd("%s:\n", s->name);
	}
	// must be constant value
	if (t->node != AST_CNST)
		assert(!"TODO");
	switch (s->ty->cat) {
	case CAT_CHAR:
	case CAT_UCHAR:
		zpd("\t.byte\t%d\n", t->u.cnst->u.cnst.c);
		break;
	case CAT_SHORT:
	case CAT_USHORT:
	case CAT_INT:
	case CAT_UINT:
	case CAT_FLOAT:
		zpd("\t.long\t%d\n", t->u.cnst->u.cnst.i);
		break;
	case CAT_DOUBLE:
		zpd("\t.long\t%d\n", t->u.cnst->u.cnst.i);
		zpd("\t.long\t%d\n", *(int *)(t->u.cnst->u.cnst.a + 4));
		break;
	case CAT_POINTER:
		if (t->ty->cat == CAT_ARRAY)	// must be a string
			zpd("\t.long\t%s\n", t->u.cnst->name);
		else
			zpd("\t.long\t%d\n", t->u.cnst->u.cnst.i);
		break;
	case CAT_STRUCT:
	case CAT_UNION:
		assert(!"TODO");
	case CAT_ARRAY:
		
	default:
		error(-1, t->line_no, "illegal initialization\n");
	}
	s->is_emitted = 1;

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void gas_data() {
	slist *p = _F.data_ids;
	symbol *s;
	
	if (p)
		zpd(".data\n");
	/* statics, stings, etc*/
	for (; p; p = p->next) {
		s = (symbol *)p->it;
		if (s->iz)	/// .data
			gas_izer(s);
		else {		/// .bss
			if (s->scope > GLOBAL)		// no linkage identifier
				// .lcomm name,size,align
				zpd(".lcomm\t%s,%d\n", s->asm_name, s->ty->size);
			else {
				if (s->is_internal)
					zpd(".lcomm\t%s,%d\n", s->name, s->ty->size);
				else if (s->is_external)
					zpd(".comm\t%s,%d\n", s->name, s->ty->size);
				else
					assert(0);
			}
		}
	}
	//fflush(f);
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void gas_file_head(FILE *f, char *name) {
	//FILE *f = asm_file;
	fprintf(f, "# this file is generated by zcc\n");
	fprintf(f, ".file\t%s\n", name);
	
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void gas_glue(icode *ic) {
	zbuf_init(buf_text, 4096);
	zbuf_init(buf_data, 2048);
	zbuf_init(buf_rodata, 2048);
	zbuf_init(buf_bss, 1024);
	//gas_file_head(asm_file);
	gas_data();
	gas_text(ic);
	gas_rodata();
	zbuf_write_free(asm_file, buf_data);
	zbuf_write_free(asm_file, buf_rodata);
	zbuf_write_free(asm_file, buf_bss);
	zbuf_write_free(asm_file, buf_text);
	fflush(asm_file);

	return;
}
