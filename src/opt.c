/**
Copyright (C) 2013
	Writed by zet
*/

#include "zcchdr.h"
#include "stdhdr.h"

/*===---------------------------------------------------------------------------
chenge code from:
	jump lab1
	...
	jump lab2
lab1:
lab2:
	instruction;

to

	jump lab1
	...
	jump lab1
lab1:
	instruction;
---------------------------------------------------------------------------===*/
static void fold_labels(icode *head) {
	icode *p = head, *cur_ic, *br;
	symbol *dst, *s;
	dlist *d;

	do {
		if (p->node == IC_LABEL) {
			cur_ic = p;
			dst = p->left;	// label will be exist
			// continous labels define
			while ((p = p->next)->node == IC_LABEL) {
				s = p->left;
				d = s->u.l.refs_ic;
				// label referenced by more than one icode
				assert(d);
				do {
					br = (icode *)d->it;
					br->left = dst;
					d = d->next;
				} while (d != s->u.l.refs_ic);
			}
			assert(p != _F.root_ic);
			/* delete useless label definition
			p is point to the first icode which is not IC_LABEL*/
			cur_ic->next = p;
			p->prev = cur_ic;
		}
		// p must not be IC_LABEL
		p = p->next;
	} while (p);

	return;
}

/*===---------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void opt_ic(icode *ic) {
	fold_labels(ic);

	return;
}
