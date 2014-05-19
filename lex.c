/**
Copyright (C) 2013
	Writed by zet
*/

#include "stdhdr.h"
#include "zcchdr.h"

#define MAX_LINE	 1024
#define is_blank(t)	(t == ' ' || t == '\t')
#define is_newline(t) (t == '\n')

//extern symbol *look_up(char *, int);
int tk = 0;
static char *pc;			/*pointer to the current character*/
static char *pl;			/*pointer to the last avaliable charater + 1*/
//struct symbol_t *lex_data.sym;				/*current token's symbol infomation, if it has*/
int zcc_line_no = 1;							/*global line number*/
static char buffer_lex[LEX_BUF_SIZE];
/* buffer, shared whith other file ?*/
char buffer_256[256];
char buffer_128[128];
//static int have_init_lex = 0; 
int get_tk();

/*===---------------------------------------------------------------------------------------

---------------------------------------------------------------------------------------===*/
void fill_buf() {
	char *i, *dst = buffer_lex;
	int ret = 0, len = 0;

	// content in buffer is enough or all of source file is read into the buffer
	if (pl - pc >= MAX_LINE || pl - buffer_lex != LEX_BUF_SIZE)
		return;
	for (i = pc; i < pl;)
		*dst++ = *i++;
	pc = buffer_lex;
	// max size of the empty buffer
	len = LEX_BUF_SIZE - (dst - buffer_lex);
	ret = fread(dst, 1, len, src_file);
	pl = buffer_lex + len + ret;
	//if (feof(src_file))
		//return;
	return;
}

/*===---------------------------------------------------------------------------------------
every time encounter the newline will test and call fill_buf(), i think this will work well
---------------------------------------------------------------------------------------===*/
static void newline() {
	zcc_line_no++;
	if (pl - pc < MAX_LINE && pl - buffer_lex == LEX_BUF_SIZE)
		fill_buf();

	return;
}

/*===---------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------===*/
void set_pc_terrible(char *p) {
	pc = p;
	return;
}

/*===---------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------===*/
char *get_pc_terrible() {
	/* this is important*/
	fill_buf();

	return pc;
}

/*===---------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------===*/
int peek() {
	char *p = get_pc_terrible();
	int tk_tmp = tk;
	int t = get_tk();
	set_pc_terrible(p);
	tk = tk_tmp;

	return t;
}

/*===----------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------===*/
void init_lex() {
	int len = 0;
	//if (have_init_lex == 1)
		//return;
	assert(src_file);
	/*the first time to fill the buffer, so the fread should not return 0*/
	len = fread(buffer_lex, 1, LEX_BUF_SIZE, src_file);
	if (len == 0)
		error(JE_EMPTY_FILE, -1, NULL);
	pc = buffer_lex;
	pl = buffer_lex + len;
	//pc++;
	while ((is_blank(*pc) || is_newline(*pc)) && pc < pl) {
		if (is_newline(*pc))
			newline();
		pc++;
	}
	//have_init_lex = 1;

	return;
}

#define is_oct(t) ((t) >= '0' && (t) <= '7')
#define is_hex(t) ((t) >= '0' && (t) <= '9' || (t) >= 'a' && (t) <= 'f' || (t) >= 'A' && (t) <= 'F')

/*===--------------------------------------------------------------------------------------
deal with the escape sequence, parameter pc pointer to the character after '\'
this function depend on the enveriment compilied zcc
--------------------------------------------------------------------------------------===*/
static char _zcc_esp() {
	char ret = 0;
	char c = *pc;

	switch (c) {
		case 'a':
			return (pc++, '\a');
		case 'b':
			return (pc++, '\b');
		case 'f':
			return (pc++, '\f');
		case 'n':
			return (pc++, '\n');
		case 'r':
			return (pc++, '\r');
		case 't':
			return (pc++, '\t');
		case 'v':
			return (pc++, '\v');
		case '\'':
			return (pc++, '\'');
		case '"':
			return (pc++, '"');
		case '\\':
			return (pc++, '\\');
		case '\?':
			return (pc++, '\?');
		default:
			if (is_oct(*pc)) {
				/*standard said has three octal number mostly, so this is a future of zcc*/
				for (; is_oct(*pc); pc++)
					ret = (ret << 3) + (*pc - '0');
				if (ret > UCHAR_MAX)
					error(JE_LEXICAL, zcc_line_no, NULL);
				else
					return ret;
			} else if (*pc == 'x') {
				for (pc++; isxdigit(*pc); pc++)
					if (*pc <= '9')
						ret = (ret << 4) + (*pc - '0');
					else if (*pc <= 'F')
						ret = (ret << 4) + (*pc - 'A') + 10;
					else
						ret = (ret << 4) + (*pc - 'a') + 10;
				if (ret > UCHAR_MAX)
					error(JE_LEXICAL, zcc_line_no, NULL);
				else
					return ret;
			} else
				error(JE_LEXICAL, zcc_line_no, NULL);
	}
}

/*===---------------------------------------------------------------------------------------
implenebtation the semantic : each member of source character set convert to 
	a member of execution character set  
----------------------------------------------------------------------------------------===*/
char char_map() {
	char *p = pc;
	// escape chracter
	if (*p == '\\') {
		pc++;
		return _zcc_esp();
	} else if (*p++ == '\n')
		error(-1, zcc_line_no, "unexpect newline\n");
	// always point to next character
	pc++;

	return *p;
}

/*===---------------------------------------------------------------------------------------
get_tk--the main function of lexical anylise
when calling this pc pointer to the character being process
----------------------------------------------------------------------------------------===*/
int get_tk() {
	char *ppc = pc++;	//pointer to previous character

	while (ppc < pl) {
		switch (*ppc) {
			case '\r':
				ppc = pc++;
				assert(*ppc == '\n');
			case '\n':
				newline();
				ppc = pc++;
				continue;

			case ' ':
			case '\t':
				ppc = pc++;
				continue;
			
			case 'a':
				if (*pc == 'u' && pc[1] == 't' && pc[2] == 'o' && is_blank(pc[3])) {
					//lex_data.sym = look_up("auto");
					pc += 3;
					return TK_AUTO;
				}
				goto id;

			case 'b':
				if (*pc == 'r' && pc[1] == 'e' && pc[2] == 'a' && pc[3] == 'k' && is_blank(pc[4])) {
					pc += 4;
					return TK_BREAK;
				}
				goto id;
			
			case 'c':
				if (*pc == 'a' && pc[1] == 's' && pc[2] == 'e' && is_blank(pc[3])) {
					pc += 3;
					return TK_CASE;
				} else if (*pc == 'h' && pc[1] == 'a' && pc[2] == 'r' && is_blank(pc[3])) {
					pc += 3;
					//lex_data.ty = char_type;
					return TK_CHAR;
				} else if (*pc == 'o' && pc[1] == 'n' && pc[2] == 't' && pc[3] == 'i' &&
					pc[4] == 'n' && pc[5] == 'u' && pc[6] == 'e'&& is_blank(pc[7])) {
					pc += 7;
					return TK_CONTINUE;
				}
				goto id;

			case 'd':
				if (*pc == 'e' && pc[1] == 'f' && pc[2] == 'a' && pc[3] == 'u' &&
					pc[4] == 'l' && pc[5] == 't' && is_blank(pc[6])) {
					pc += 6;
					return TK_DEFAULT;
				} else if (*pc == 'o' && pc[1] == 'u' && pc[2] == 'b' && pc[3] == 'l' &&
					pc[4] == 'e' && is_blank(pc[5])) {
					pc += 5;
					//lex_data.sym = look_up(types_tb, "double", GLOBAL);
					return TK_DOUBLE;
				} else if (*pc == 'o' && is_blank(pc[1])) {
					pc++;
					//ppc = pc++;
					return TK_DO;
				}
				goto id;

			case 'e':
				if (*pc == 'l' && pc[1] == 's' && pc[2] == 'e' && is_blank(pc[3])) {
					pc += 3;
					return TK_ELSE;
				} else if (*pc == 'x' && pc[1] == 't' && pc[2] == 'e' && pc[3] == 'r' &&
					pc[4] == 'n' && is_blank(pc[5])) {
					pc += 5;
					return TK_EXTERN;
				} else if (*pc == 'n' && pc[1] == 'u' && pc[2] == 'm'&& is_blank(pc[3])) {
					pc += 3;
					return TK_ENUM;
				}
				goto id;
			
			case 'f':
				if (*pc == 'l' && pc[1] == 'o' && pc[2] == 'a' && pc[3] == 't' && is_blank(pc[4])) {
					pc += 4;
					//lex_data.sym = look_up(types_tb, "float", GLOBAL);
					return TK_FLOAT;
				} else if (*pc == 'o' && pc[1] == 'r' && is_blank(pc[2])) {
					pc += 2;
					return TK_FOR;
				}
				goto id;

			case 'g':
				if (*pc == 'o' && pc[1] == 't' && pc[2] == 'o' && is_blank(pc[3])) {
					pc += 3;
					return TK_GOTO;
				}
				goto id;
			
			case 'i':
				if (*pc == 'f' && is_blank(pc[1])) {
					pc++;
					return TK_IF;
				/* pc[2] != character*/
				} else if (*pc == 'n' && pc[1] == 't' && is_blank(pc[2])) {
					pc += 2;
					//lex_data.sym = look_up(types_tb, "int", GLOBAL);
					return TK_INT;
				}
				goto id;
			
			case 'l':
				if (*pc == 'o' && pc[1] == 'n' && pc[2] == 'g' && is_blank(pc[3])) {
					pc += 3;
					//lex_data.sym = look_up(types_tb, "long", GLOBAL);
					return TK_LONG;
				}
				goto id;

			case 'r':
				if (*pc == 'e' && pc[1] == 't' && pc[2] == 'u' && pc[3] == 'r' && 
					pc[4] == 'n' && is_blank(pc[5])) {
					pc += 5;
					return TK_RETURN;
				} else if (*pc == 'e' && pc[1] == 'g' && pc[2] == 'i' && pc[3] == 's' &&
					pc[4] == 't' && pc[5] == 'e' && pc[6] == 'r' && is_blank(pc[7])) {
					pc += 7;
					return TK_REGISTER;
				}
				goto id;

			case 's':
				if (*pc == 'h' && pc[1] == 'o' && pc[2] == 'r' && pc[3] == 't' && is_blank(pc[4])) {
					pc += 4;
					//lex_data.sym = look_up(types_tb, "short", GLOBAL);
					return TK_SHORT;
				} else if (*pc == 'i' && pc[1] == 'g' && pc[2] == 'n' && pc[3] == 'e' && 
					pc[4] == 'd' && is_blank(pc[5])) {
					pc += 5;
					return TK_SIGNED;
				} else if (*pc == 'i' && pc[1] == 'z' && pc[2] == 'e' && pc[3] == 'o' && 
					pc[4] == 'f' && is_blank(pc[5])) {
					pc += 5;
					return TK_SIZEOF;
				} else if (*pc == 't' && pc[1] == 'a' && pc[2] == 't' && pc[3] == 'i' && 
					pc[4] == 'c' && is_blank(pc[5])) {
					pc += 5;
					return TK_STATIC;
				} else if (*pc == 't' && pc[1] == 'r' && pc[2] == 'u' && pc[3] == 'c' && 
					pc[4] == 't' && is_blank(pc[5])) {
					pc += 5;
					return TK_STRUCT;
				} else if (*pc == 'w' && pc[1] == 'i' && pc[2] == 't' && pc[3] == 'c' && 
					pc[4] == 'h' && is_blank(pc[5])) {
					pc += 5;
					return TK_SWITCH;
				}
				goto id;

			case 't':
				if (*pc == 'y' && pc[1] == 'p' && pc[2] == 'e' && pc[3] == 'd' &&
					pc[4] == 'e' && pc[5] == 'f' && is_blank(pc[6])) {
					pc += 6;
					return TK_TYPEDEF;
				}
				goto id;

			case 'u':
				if (*pc == 'n' && pc[1] == 'i' && pc[2] == 'o' && pc[3] == 'n' && is_blank(pc[4])) {
					pc += 4;
					return TK_UNION;
				} else if (*pc == 'n' && pc[1] == 's' && pc[2] == 'i' && pc[3] == 'g' &&
					pc[4] == 'n' && pc[5] == 'e' && pc[6] == 'd' && is_blank(pc[7])) {
					pc += 7;
					return TK_UNSIGNED;
				}
				goto id;
			
			case 'v':
				if (*pc == 'o' && pc[1] == 'i' && pc[2] == 'd' && is_blank(pc[3])) {
					pc += 3;
					return TK_VOID;
				} else if (*pc == 'o' && pc[1] == 'l' && pc[2] == 'a' && pc[3] == 't' &&
					pc[4] == 'i' && pc[5] == 'l' && pc[6] == 'a' && is_blank(pc[7])) {
					pc += 7;
					return TK_VOLATILE;
				}
				goto id;
			
			case 'w':
				if (*pc == 'h' && pc[1] == 'i' && pc[2] == 'l' && pc[3] == 'e' && is_blank(pc[4])) {
					pc += 4;
					return TK_WHILE;
				}
				goto id;

			case '0': case '1': case '2': case '3': case'4': 
			case '5': case '6': case'7': case '8': case '9': {
				type *type = NULL;
				char *index = pc, *p = pc;
				double d;
				//float f;
				long l;
				value v;
				int is_float = 0, is_double = 0;

				while (!(is_blank(*index) || is_newline(*index) || *index == '.' || *index == 'e' || *index == 'E'))
					index++;
				/* float number*/
				if (*index == '.' || *index == 'e' || *index == 'E') {
					is_double = 1;
					errno = 0;
					d = strtod(ppc, &p);
					if (errno == ERANGE)
						error(JE_OVERFLOW, zcc_line_no, NULL);
				/* integer number*/
				} else {
					//is_float = 0;
					errno = 0;
					l = strtol(ppc, &p, 0);
					if (errno == ERANGE)
						error(JE_OVERFLOW, zcc_line_no, NULL);
				}
				/* test like the float 3f*/
				if (*p == 'f'|| *p == 'F') {
					is_float = 1;
					p++;
				}
				/* Attention : update pc*/
				pc = p;
				/* now deal with the constant*/
				if (is_float && is_double) {
					if (d > FLT_MAX || d < FLT_MIN)
						error(JE_OVERFLOW, zcc_line_no, NULL);
					v.f = (float)d;
					lex_data.sym = constant(v, float_type);
					return TK_FCNST;
				} else if (is_float && is_double == 0) {
					v.f = (float)l;
					lex_data.sym = constant(v, float_type);
					return TK_FCNST;
				} else if (is_float == 0 && is_double) {
					v.d = d;
					lex_data.sym = constant(v, double_type);
					return TK_DCNST;
				} else {
					if (l > INT_MAX || l < INT_MIN)
						warn(-1, zcc_line_no, "constant overflow\n");
					v.i = l;
					lex_data.sym = constant(v, int_type);
					return TK_ICNST;
				}
			}
			/* character constant*/
			case '\'': {
				value v;
				char ch = char_map();
				if (ch == -1 || ch == '\'' || *pc != '\'')
					error(JE_LEXICAL, zcc_line_no, NULL);
				v.i = ch;
				lex_data.sym = constant(v, int_type);
				pc++;	//pointer to the character after single quotation
				return TK_ICNST;
			}
			/* string literals*/
			case '"': {
				char val;
				char str_buf[MAX_LINE];
				char *p = str_buf, *tmp;
				int len;
				value v;
				symbol *sym;
				//type *ty;

				while (1) {
					while ((*p++ = char_map()) != '"')	/*char_map() will increase the pc*/
						;
					// current value of p point is '\"', so need eat it
					p--;
					// eat blank and newline
					for (tmp = pc; is_blank(*tmp) || *tmp == '\r' || *tmp == '\n'; tmp++)
						if (*tmp == '\n') {
							pc = tmp;
							newline();
							tmp = pc;
						}
					// no adjacent string need concatenate
					if (*tmp != '"') {
						pc = tmp;
						break;
					}
				}

				*p = 0;
				len = p - str_buf;
				/* add the litral in strings table which will be output into ASM file*/
				p = ins_str(str_buf, len);
				/* balck magic,
				for the homogeneity of all the constant's define and reference code*/
				//*(int *)(v.a + 4) = len;
				//ty = clone_type(char_type, PERM);
				/* the literal should be constant*/
				//ty->is_const = 1;
				v.p = p;
				sym = constant(v, new_array(char_type, len, PERM));
				sym->name = gen_tmp_name("LI");
				add2slist_head(&_F.rodata_strings, sym, PERM);
				lex_data.sym = sym;

				return TK_SCNST;
			}

			/* TODO: ++, --, ->, >>, <<, +=...*/
			case '+':
				if (*pc == '+') {
					pc++;
					return TK_INC;
				} else if (*pc == '=') {
					pc++;
					return TK_ADD_ASGN;
				} else
					return *ppc;
			case '-':
				if (*pc == '-') {
					pc++;
					return TK_DEC;
				} else if (*pc == '>') {
					pc++;
					return TK_ARROW;
				} else if (*pc == '=') {
					pc++;
					return TK_SUB_ASGN;
				} else
					return *ppc;
			case '&':
				if (*pc == '&') {
					pc++;
					return TK_AND;
				} else if (*pc == '=') {
					pc++;
					return TK_BAND_ASGN;
				} else
					return *ppc;
			case '|':
				if (*pc == '|') {
					pc++;
					return TK_OR;
				} else if (*pc == '=') {
					pc++;
					return TK_BIOR_ASGN;
				} else
					return *ppc;
			case '^':
				if (*pc == '=') {
					pc++;
					return TK_BXOR_ASGN;
				} else
					return *ppc;
			case '=':
				if (*pc == '=') {
					pc++;
					return TK_EQU;
				} else
					return *ppc;
			case '!':
				if (*pc == '=') {
					pc++;
					return TK_UE;
				} else
					return *ppc;
			case '*':
				if (*pc == '=') {
					pc++;
					return TK_MUL_ASGN;
				} else
					return *ppc;
			case '/':
				if (*pc == '=') {
					pc++;
					return TK_DIV_ASGN;
				} else
					return *ppc;
			/* <<, <=, <<=*/
			case '<':
				if (*pc == '<')
					if (pc[1] == '=') {
						pc += 2;
						return TK_LS_ASGN;
					} else {
						pc++;
						return TK_LS;
					}
				else if (*pc == '=') {
					pc++;
					return TK_LE;
				} else
					return *ppc;
			/* >>, >=, >>=*/
			case '>':
				if (*pc == '>')
					if (pc[1] == '=') {
						pc += 2;
						return TK_RS_ASGN;
					} else {
						pc++;
						return TK_RS;
					}
				else if (*pc == '=') {
					pc++;
					return TK_GE;
				} else
					return *ppc;
			
			case '.':
				if (*pc == '.' && pc[1] == '.' && pc[2] != '.')
					pc += 2;
				return 
					TK_ELPS;
			/* if '\' is not part of escape sequences, is it possible?? */
			case '\\':
				assert(0);
				//return escape();
#if 0
			case '!': case '&': case '#': case '%': case '(': case ')': case '*': case '+':
			case ',': case '-': case '/': case ':': case ';': case '<': case '=': case '?': 
			case '[': case ']': case '^': case '{': case '}': case '|': case '~':
				pc++;
				return *ppc;
#endif

id:	
			case '_': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': 
			case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': 
			case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W':
			case 'X': case 'Y': case 'Z': case 'h': case 'j': case 'k': case 'm': case 'n':
			case 'o': case 'p': case 'q': case 'x': case 'y': case 'z': {
				char *p = buffer_128, *name;
				symbol *s = NULL;
				*p++ = *ppc;
				while (isalnum(*pc) || *pc == '_' )
					*p++ = *pc++;
				*p = 0;
				name = simple_string(buffer_128);
				s = look_up(ids_tb, name);
				if (s && s->is_typedef) {
					lex_data.ty = s->ty;
					return TK_TYPE;
				} else {
					lex_data.name = name;
					return TK_ID;
				}
			}
			default:
				return *ppc;

		}	//switch
	}

	return TK_EOF;
}
