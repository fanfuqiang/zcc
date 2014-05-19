/*==----------------------------------------------------------------------
inst(ic, value, gas_inst, print_func)
	if want port to other assembler, add instruction to this file
-----------------------------------------------------------------------==*/
inst(IC_NOP,			0,	NULL,		NULL)
inst(IC_FUNC_BEGIN,		1,	NULL,		gas_prolog)
inst(IC_JMP,			2,	"jmp",		gas_branch)
inst(IC_JE,				3,	"je",		gas_branch)
inst(IC_JNE,			4,	"jne",		gas_branch)
inst(IC_JLE,			5,	"jle",		gas_branch)
inst(IC_JL,				6,	"jl",		gas_branch)
inst(IC_CALL,			7,	"call",		gas_branch)
inst(IC_LABEL,			8,	NULL,		gas_label)
inst(IC_CMP,			9,	"cmp",		gas_cmp)
inst(IC_CMP_0,			10,	"cmp",		gas_cmp)	// cmp op1, 0
inst(IC_CMP_1,			11,	"cmp",		gas_cmp)	// cmp op1, 1
/* move value*/
inst(IC_MOV,			12,	"mov",		gas_mov)
inst(IC_MOV_BLOCK,		13,	NULL,		gas_mov_block)
inst(IC_MOV_ARG,		14,	NULL,		gas_mov_arg)
/* convert*/
inst(IC_CVT,			15,	NULL,		gas_cvt)
/* address*/
inst(IC_ADDR,			16,	NULL,		gas_addr)
inst(IC_MUL,			17,	"imul",		gas_bin)
inst(IC_DIV,			18,	"idiv",		gas_bin)
inst(IC_MOD,			19,	"mod",		gas_bin)
inst(IC_ADD,			20,	"add",		gas_bin)
inst(IC_SUB,			21,	"sub",		gas_bin)
inst(IC_LS,				22,	"sal",		gas_bin)
inst(IC_RS,				23,	"sar",		gas_bin)
inst(IC_BAND,			24,	"and",		gas_bin)
inst(IC_BXOR,			25,	"xor",		gas_bin)
inst(IC_BIOR,			26,	"or",		gas_bin)
inst(IC_INC,			27,	"inc",		gas_inc)
inst(IC_DEC,			28,	"dec",		gas_inc)
inst(IC_NEG,			29,	"neg",		gas_unary)
inst(IC_NOT,			30,	"not",		gas_unary)
inst(IC_SETE,			31,	"sete",		gas_unary)
inst(IC_FUNC_END,		32,	NULL,		gas_epilog)
inst_last(IC_SENTRY,	99,	NULL,		NULL)
