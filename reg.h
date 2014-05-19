/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __REG_H__
#define __REG_H__
/* define register index constant*/
enum reg_index {
	/* 8 bits*/
	//AL,
	//BL,
	//CL,
	//DL,
	//AH,
	//BH,
	//CH,
	//DH,
	/* 32 bits*/
	EAX,
	EBX,
	ECX,
	EDX,
	ESI,
	EDI,
	/* resreved*/
	EBP,
	ESP
};
extern struct symbol_t f80_regs[];
extern struct symbol_t i32_regs[];
/* interface*/
void init_regs();
struct symbol_t *need_reg(int idx);
struct symbol_t *get_reg(int kind);
struct symbol_t *get_spill_loc(int kind);
void enrol_reg(struct symbol_t *reg, struct symbol_t *s);
void relieve_reg(struct symbol_t *reg, struct symbol_t *s);
void relieve_loc(struct symbol_t *loc, struct symbol_t *s);
void free_spill_loc(struct symbol_t *s);
void alter(struct symbol_t *s);
void spill(struct symbol_t *r);
//int is_has_reg(struct symbol_t *s);
void alloc_reg(struct symbol_t *s);
#endif
