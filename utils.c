/**
Copyright (C) 2013
	Writed by zet
*/

#include"stdhdr.h"

#include"zcchdr.h"



/*===-----------------------------------------------------------------

this alogrithm is based on Peter Weinberger's generic hash.  

-----------------------------------------------------------------===*/
int str2key(const char *p, int length, int hsize) {
	int key = 0, l, g;
	assert(*p);
	for (l = 0; l < length; l++) {
		key = (key << 4) + *p++;
		if (g = key & 0xf0000000)
			key ^= g >> 24;
		key &= ~g;
	}


	return (key % (hsize-1));
}



/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/

void slist_delete_item(slist **head, void *it) {
	slist *p, **pp = head;

	if (*head == NULL)
		return;
	if ((p = *head) && p->it == it) {
		*head = p->next;
		return;
	}
	/* not the first item*/

	for (; p = *pp; pp = &p->next) {
		if (p->it == it) {
			*pp = p->next;
			return;
		}
	}
	assert(0);
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
void *get_slist_head(slist **list_addr) {
	slist *p = *list_addr;
	assert(p);
	*list_addr = p->next;
	
	return p->it;
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
void add2slist_head(slist **l, void *it, int area) {
	slist *lst;

	NEW0(lst, area);
	lst->it = it;
	lst->next = *l;
	//lst->prev = *l;
	*l = lst;
	//(*l)->next->prev = lst;

	return;
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
void insert2dlist(dlist **l, void *it, int area) {
	dlist *lst;

	NEW0(lst, area);
	lst->it = it;
	assert(*l);
	lst->next = (*l)->next;
	(*l)->next->prev = lst;
	(*l)->next = lst;
	lst->prev = *l;

	return;
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
void add2slist_tail(slist **l, void *it, int area) {
	slist *lst, **pp = l;

	NEW0(lst, area);
	lst->it = it;
	while (*pp)
		pp = &(*pp)->next;
	*pp = lst;

	return;
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
static int is_has_dup(dlist *h, void *it) {
	dlist *d = h;
	
	if (!h)
		return 0;
	do {
		if (d->it == it)
			return 1;
		d = d->next;
	} while (d != h);

	return 0;
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
void add2dlist_head(dlist **l, void *it, int area) {
	dlist *lst;

	if (is_has_dup(*l, it))
		return;
	NEW0(lst, area);
	lst->it = it;
	if (*l) {
		lst->next = *l;
		lst->prev = (*l)->prev;
		(*l)->prev->next = lst;
		(*l)->prev = lst;
		/* change the source value*/
		*l = lst;
	} else {
		*l = lst;
		(*l)->prev = lst;
		(*l)->next = lst;
	}
	return;
}

/*===-------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void dlist_delete_item(dlist **dd, void *it) {
	dlist *d = *dd;
	if (d->next == d && d->it == it) {
		*dd = NULL;
		return ;
	}
	do {
		if (d->it == it) {
			d->next->prev = d->prev;
			d->prev->next = d->next;
			if (d == *dd)
				*dd = d->next;
			return;
		}
		d = d->next;
	} while (d != *dd);

	assert(!"compiler internal relieve_reg error");
}

/*===-----------------------------------------------------------------------

------------------------------------------------------------------------===*/
void add2dlist_tail(dlist **dd, void *it, int area) {
	dlist *d;

	if (is_has_dup(*dd, it))
		return;
	NEW0(d, area);
	d->it = it;
	if (*dd) {
		// this dlist has content
		d->next = *dd;
		d->prev = (*dd)->prev;
		(*dd)->prev->next = d;
		(*dd)->prev = d;
	} else {
		// empty dlist
		*dd = d;
		d->next = d;
		d->prev = d;
	}

	return;
}
#if 0
/*===------------------------------------------------------------------------

-------------------------------------------------------------------------===*/
char *list2vec(slist *lst) {
	slist *it = lst;
	int length = 0;
	char *vec = NULL;
	do
		length += (strlen(it->it) + 2);
	while(it->next);

	vec = zcc_alloc(length, PERM);

	it = lst;
	do {
		int len = strlen(it->it);
		strncpy(vec, it->it, len + 1);
		//*(vec + len) = NULL;
		*(vec + len + 1) = 32; //' ' = 32
		vec = vec + len + 2;
	} while(it->next);
	
	return vec;
}

/*===-------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
slist *reverse(slist **l) {
	//List *it = NULL;
	slist *prev = NULL, *curr = NULL, *next = NULL;
	assert(*l);

	prev = *l;
	//it = &(*slist)->next;
	for (curr = prev->next; curr; curr = next) {
		next = curr->next;
		curr->next = prev;
		prev = curr;
	}
	if ((*l)->next)
		*l = curr;

	return *l;
}
#endif
/*===---------------------------------------------------------------------------
#define __align(x, n) ((x+((n)-1))&(~((n)-1)))
----------------------------------------------------------------------------===*/
void _align(int *val, int n) {
	*val = (*val + (n-1)) & ~(n-1);

	return;
}

/*===----------------------------------------------------------------------------

-----------------------------------------------------------------------------===*/
void del_head_node(slist **ls) {
	if (*ls == NULL)
		return;
	else
		*ls = (*ls)->next;

	return;
}
