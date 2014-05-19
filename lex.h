/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __LEX_H__
#define __LEX_H__

#define H_SIZE 127
#define LEX_BUF_SIZE 4096
/* current line number*/
extern int zcc_line_no;
/*used by get the scanner's return value*/
extern int tk;
/* token value*/
enum {
#define native(a, b, c, d)
#define token(a, b, c, d) a=b,
#include "token.h"
#undef native
#undef token
};
/* buffer*/
extern char buffer_256[];
extern char buffer_128[];
/* interface*/
void init_lex();
int get_tk();
int peek();
void fill_buf();
void set_pc_terrible(char *p);
char *get_pc_terrible();

#endif