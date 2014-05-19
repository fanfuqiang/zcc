/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __ALLOC_H__
#define __ALLOC_H__

enum {
	a_undef,
	STMT,
	AST = STMT,
	IC = STMT,
	FUNC,
	TY = FUNC,		// type
	PERM
};

#define AELEMS(a) ((int)(sizeof(a)/sizeof((a)[0])))

void *zcc_alloc(int n, int a);
void zcc_free(int a);
#define NEW(p, a) ((p) = zcc_alloc(sizeof *(p), (a)))
#define NEW0(p, a) memset(NEW((p),(a)), 0, sizeof *(p))

#endif