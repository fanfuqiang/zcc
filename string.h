/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __STRING_H__
#define __STRING_H__

union align {
	double d;
	int (*pf) ();
};

char *ins_str(char *str, int len);
char *simple_string(char *str);

#endif