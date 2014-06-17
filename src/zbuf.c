/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static int eval_size(char *format, va_list args) {
	char *p = format;
	//va_list vp = args;
	int size = strlen(p);

	while (*p) {
		if (*p++ == '%') {
			if (*p == 'd') {
				size += 20;		/* the size of INT_MIN is less than 20*/
				va_arg(args, int);
			} else if (*p == 's')
				size += strlen(va_arg(args, char *));
			//else
				//assert(0);
		}
	}

	return size;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void zbuf_realloc(zbuf *buf, int size) {
	if (buf->used_size + size < buf->total_size)
		return;
	/* count the total size*/
	do {
		buf->total_size += buf->total_size;
	} while (buf->total_size > buf->used_size + size);
	if ((buf->base = (char *)realloc(buf->base, buf->total_size)) == NULL)
		error(-1, 0, "realloc failed\n");

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void zbuf_vprint(zbuf *buf, char *format, va_list args) {
	int size = eval_size(format, args);
	int len;

	assert(buf->total_size);
	assert(size > 0);
	zbuf_realloc(buf, size);
	len = vsprintf(buf->base + buf->used_size, format, args);
	if (len >= 0) {
		assert (len <= size);
		buf->used_size += len;
	}

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void zbuf_print(zbuf *buf, char *format, ...) {
	va_list arg;
	va_start(arg, format);
	zbuf_vprint(buf, format, arg);
	va_end(arg);

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void zbuf_init(zbuf *buf, int size) {
	assert(size > 0);
	
	buf->used_size = 0;
	buf->total_size = size;
	if ((buf->base = (char *)malloc(size)) == NULL)
		error(-1, 0, "malloc failed\n");

	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void zbuf_write_free(FILE *f, zbuf *buf) {
	if (buf->used_size) {
		fwrite(buf->base, 1, buf->used_size, f);
		free(buf->base);
		buf->base = NULL;
		buf->total_size = 0;
		buf->used_size = 0;
	}

	return;
}
