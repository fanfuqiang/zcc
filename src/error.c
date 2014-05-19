/**
Copyright (C) 2013
	Writed by zet
*/

#include"zcchdr.h"
#include"stdhdr.h"
/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
char *get_error_msg(int code) {
	switch (code) {
		case JE_NOMEM:
			return "insufficient memory";
		case JE_USER:
			return "Error general purpose register";
		case JE_ILLARG:
			return "Illgal arguments";
		case JE_NOINF:
			return "Did not appoint the input source file";
		case JE_EMPTY_FILE:
			return "the file being compiled is empty";
		case JE_OVERFLOW:
			return "overflow happened";
		case JE_LEXICAL:
			return "lexical error";
		case JE_UNKNOWN:
		default:
			return "UNKNOWN";
	}
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void error(int code, int line_no, char *format, ...) {
	char *s;
	int line = -1;

	if (format == NULL)
		s = get_error_msg(code);
	else {
		va_list args;
		va_start(args, format);
		vsprintf(buffer_256, format, args);
		va_end(args);
		s = buffer_256;
	}

	if (line_no)
		line = line_no;
	if (!_S.is_Q)
			fprintf(stderr, "%s:%d:Error [%03d] %s\n", src_file_name,
			line, code, s);
	exit(1);

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void warn(int code, int line_no, char *message) {
	int line = -1;

	assert(message);
	if (line_no)
		line = line_no;
	if (!_S.is_Q)
		printf("%s:%d:Warning [%03d] %s\n", src_file_name,
				line, code, message);

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
void panic(int line) {
	fprintf(stderr, "line: %d zcc internal error\n", line);
	exit(EXIT_FAILURE);
}
