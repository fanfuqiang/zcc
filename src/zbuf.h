/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __BUFFER_H__
#define __BUFFER_H__
#include <stdio.h>
#include <stdarg.h>

/* buffer struct*/
typedef struct zbuf_t {
	int used_size;		/* size of used buffer*/
	int total_size;		/* total size of the buffer*/
	char *base;
} zbuf;

/* interface*/
void zbuf_init(zbuf *buf, int size);
void zbuf_print(zbuf *buf, char *format, ...);
void zbuf_write_free(FILE *f, zbuf *buf);
void zbuf_vprint(zbuf *buf, char *format, va_list args);

#endif