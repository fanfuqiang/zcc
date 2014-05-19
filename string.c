/**
Copyright (C) 2013
	Writed by zet
*/

#include"stdhdr.h"
#include"zcchdr.h"

#define ST_SIZE 1022		/*because 1021 is a prime number*/

struct string_t {
	char *str;
	int len;
	struct string_t *next;
};
static struct string_t *buckets[ST_SIZE];

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
/* TODO, the string literal maybe has \0 within it*/
char *ins_str(char *str, int len) {
	struct string_t *p = NULL;
	//int len = strlen(str);
	int h = str2key(str, len, ST_SIZE);

	assert(str);
	assert(len);
	for (p = buckets[h]; p; p = p->next)
		if (len == p->len) {
			int l = len;
			char *dst = p->str;
			char *end = str;
			while (l-- && *dst++ == *end++)
				if (l == 0)
					return p->str;
		}
	{
		/*the memory pointed by str_limit is invalid*/
		static char *next, *str_limit;
		if (len + 1 >= str_limit - next) {
			int n = len + 4*1024;
			next = zcc_alloc(n, PERM);
			str_limit = next + n;
		}
		
		NEW0(p, PERM);
		p->len = len;
		for (p->str = next; len; len--)
			*next++ = *str++;
		*next++ = 0;
		p->next = buckets[h];
		buckets[h] = p;
		return p->str;
	}
}

/*===---------------------------------------------------------------------------
deal with simply string which do not has \0 inside.
----------------------------------------------------------------------------===*/
char *simple_string(char *src) {
	int len = strlen(src);

	return ins_str(src, len);
}
