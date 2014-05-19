/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <stdio.h>
#define CONTACT "feqin1023(at)gmail.com"
#define VERSION "zcc-0.0.1 bata"
/*file name*/
extern char *src_file_name;		//the C source filename being compiled
extern char *out_file_name;		//dummy 
//extern char *pp_file_name;		//the C source file after preprocess 
//extern char *asm_file_name;		//asm filename
//extern char *bin_file_name;		//the executable filename
extern FILE *src_file;
extern FILE *asm_file;
///
struct state_t {
	int is_H :1;	// help ?
	int is_Q :1;	// quiet ?
	int is_E :1;	// only doing preprocess ?
	int is_S :1;	// output ASM file ?
	int errors;		// error number
	int help_times;
	//int warnings;
	//int messages;
};
typedef struct state_t state;
extern struct state_t _S;

#endif

