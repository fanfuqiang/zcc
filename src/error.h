/**
Copyright (C) 2013
	Writed by zet
*/

#ifndef __ERROR_H__
#define __ERROR_H__

void error(int code, int line_no, char *format, ...);
void warn(int code, int line_no, char *message);
void panic(int line);
/// error code
enum JE_codes {
	JE_NOMEM = 101,
	JE_ILLARG,
	JE_NOINF,
	JE_USER,
	JE_EMPTY_FILE,
	JE_OVERFLOW,
	JE_LEXICAL,
	JE_UNKNOWN
};
///
enum JW_codes {
	JW_NOT_DEFINED = 201,
	JW_UNKNOWN
};
///
enum JM_codes {
	JM_USER = 301, /* User supplied message */
	JM_UNKNOWN
};

#endif
