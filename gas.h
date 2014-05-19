/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __GAS_H__
#define __GAS_H__

//extern char *gas_inst[];
void gas_glue(struct icode_t *ic);
//void gas_file_head();
void gas_reg_to_mem(struct symbol_t *s, struct symbol_t *reg);
int eval_kind(int cat);
void gas_mem_to_reg(struct symbol_t *reg, struct symbol_t *s);
void gas_restore_loc(struct symbol_t *s);
#endif
