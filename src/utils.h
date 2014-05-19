/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __UTILS_H__
#define __UTILS_H__

/* single list*/
typedef struct slist_t slist;
struct slist_t {
	void *it;
	slist *next;
};
/* double list*/
typedef struct dlist_t dlist;
struct dlist_t {
	void *it;
	dlist *next;
	dlist *prev;
};

int str2key(const char *p, int length, int hsize);
void _align(int *dst, int size);
/* single list*/
//void add2list(slist **l, void *it, int area);
void add2slist_head(slist **l, void *it, int area);
void add2slist_tail(slist **l, void *it, int area);
void slist_delete_item(slist **head, void *it);
void *get_slist_head(slist **list_addr);
void del_head_node(slist **ls);
/* double list*/
void add2dlist_head(dlist **l, void *it, int area);
void insert2dlist(dlist **l, void *it, int area);
void add2dlist_tail(dlist **l, void *it, int area);
void dlist_delete_item(dlist **dd, void *it);

#endif