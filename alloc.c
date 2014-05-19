/**
Copyright (C) 2013
	Writed by zet
*/

#include"stdhdr.h"
#include"zcchdr.h"

/*===------------------------------------------------------------------------
this alogrithms from the book "C interfcaces and implementations"
------------------------------------------------------------------------===*/
//union align {
	//double d;
	//int (*pf) ();
//};

struct block {
	char *avail;
	char *limit;
	struct block *next;
};

union header {
	struct block b;
	union align a;
};
/* */
static struct block
	first[] = {{ NULL, }, { NULL, }, { NULL, }, {NULL, }},
	*arena[] = {&first[0], &first[1], &first[2], &first[3]};

static struct block *free_blocks;
/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void *zcc_alloc(int n, int a) {
	struct block *ap;
	int m;

	assert(a <= PERM);
	assert(n > 0);
	ap = arena[a];
	_align(&n, sizeof(union align));
	while (n > ap->limit - ap->avail) {
		if ((ap->next = free_blocks) != NULL) {
			free_blocks = free_blocks->next;
			ap = ap->next;
		} else {
			m = sizeof(union header) + n + 10*1024;
			_align(&m, sizeof(union align));
			ap->next = malloc(m);
			ap = ap->next;
			if (ap == NULL)
				error(-1, -1, "malloc failed\n");
			ap->limit = (char *)ap + m;
		}
		ap->avail = (char *)((union header *)ap + 1);
		ap->next = NULL;
		arena[a] = ap;
	}
	ap->avail += n;
	
	return ap->avail - n;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void zcc_free(int a) {
	assert(a < AELEMS(arena));
	arena[a]->next = free_blocks;
	free_blocks = first[a].next;
	first[a].next = NULL;
	arena[a] = &first[a];

	return;
}
