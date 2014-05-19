/**
Copyright (C) 2013
	Writed by zet
*/

#include"zcchdr.h"
#include"stdhdr.h"

//#define MAX_FILE 8
//#define CMD_LEN 256
struct state_t _S;
/// files
FILE *src_file;
FILE *asm_file;
char *src_file_name;		//the C source filename being compiled
char *out_file_name;			//dummy 
//char *pp_file_name;			//the C source file after preprocess 
//char *asm_file_name;		//asm filename
//char *bin_file_name;		//the executable filename
void gas_file_head(FILE *f, char *name);

#if 0
/*===----------------------------------------------------------------------
open the input and output file
-----------------------------------------------------------------------===*/
char *pre_cmd[] = {"gcc", "-E", "-D_ZCC", "$1", "-o", "$2", 0};
char *asm_cmd[] = {"nasm", "&1", "-o", "$2", 0};
char *link_cmd[] = {"ld", "$1", "-o" "$2", 0};

/*===-------------------------------------------------------------------------
in - input filename
out - output filename
ext - extend filename
-------------------------------------------------------------------------===*/
char *build_cmd(char **pat, char* in, char* out) {
	int i = 0;
	char *line = NULL;
	char *cmd;
	slist *lst = NULL;
	for (; line = pat[i]; i++) {
		char *pc = line;
		if (*pc == '$')
			if (*++pc == '1')
				add2slist_head(&lst, in);
			else
				add2slist_head(&lst, out);
		else 
			add2slist_head(&lst, line);
	}
	lst = reverse(&lst);
	cmd = list2vec(lst);

	return cmd;
}

/*===-------------------------------------------------------------------------------

--------------------------------------------------------------------------------===*/
int call_prep() {
	char *cmd = NULL;
	char *name;
	if (out_file_name = NULL) {
		out_file_name = jstrdup(src_file_name);
		name = out_file_name = ch_ext(out_file_name, "i");
	} else
		name = out_file_name;
	cmd = build_cmd(pre_cmd, src_file_name, name);
	assert(cmd);

	return system(cmd);
}

/*===---------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
int call_as() {
	char *cmd = NULL;
	char *name = ch_ext(src_file_name, "s");
	cmd = build_cmd(link_cmd, name, out_file_name);
	assert(cmd);

	return system(cmd);
}

/*===----------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
void filenames() {
	if (src_file_name == NULL) {
		_S.errors++;
		error(JE_NOINF, NULL);
		return;
	}
	if (out_file_name == NULL)
		out_file_name = jstrdup(src_file_name);
	pp_file_name = ch_ext(src_file_name, "i");
	if (!_S.isnocomp)
		asm_file_name = ch_ext(src_file_name, "s");
	if (!_S.isnolink)

}
#endif

/*===-------------------------------------------------------------------------------
get a new name and change the extend
--------------------------------------------------------------------------------===*/
static char *adjust_file_post(char *name, char *post) {
	char *p, *buffer = buffer_128;
	int len = strlen(name);

	if (len > 128 - 1)
		if(len < 256 - 1)
			buffer = buffer_256;
		else
			assert(!"very long name");
	strcpy(buffer, name);
	for (p = buffer; *p != '.' && *p != 0; p++)
		;	// nothing to do
	if (*p == '.')
		strcpy(++p, post);
	else if (*p == '\0')
		strcpy(p, post);

	return simple_string(buffer);
}

/*===-------------------------------------------------------------------------

--------------------------------------------------------------------------===*/
static void init_files() {
	if (src_file_name)
		if (src_file = fopen(src_file_name, "rb")) {
			if (out_file_name == NULL)
				out_file_name = adjust_file_post(src_file_name, "s");
			if (!(asm_file = fopen(out_file_name, "w")))
				error(-1, -1, "failed creat output file %s\n", src_file_name);
		} else
			error(-1, -1, "failed open file %s\n", src_file_name);
	else
		error(-1, -1, "source file empty\n");

	return;	
}

/*===----------------------------------------------------------------------------

-----------------------------------------------------------------------------===*/
static void print_help() {
	if (_S.help_times)
		return;
	printf("call zcc link this:\nzcc [-option] [out_file_name] src_file_name\n");
	printf("-o name/-oname					alternate name of output file\n");
	printf("-h								print the help messeages\n");
	printf("-E								only preprocess donot compile\n");
	printf("-S								output the assembler file donot link\n");
	printf("-v								print the version\n");
	printf("bug report and advices please mailling %s\n", CONTACT);
	fflush(stdout);
}
/*===-----------------------------------------------------------------------------

------------------------------------------------------------------------------===*/
static void process_args(int argc, char **argv) {
	int i;
	char c;
	// argc >= 2
	if(argc > 1) {
		for(i = 1; i < argc; i++)
			if(argv[i][0] == '-') {
				c = argv[i][1];
				switch(c) {
					case 'o':
						if (argv[i] + 2)
							out_file_name = simple_string(argv[i] + 2);
						else
							out_file_name = simple_string(argv[++i]);
						break;
					case 'E':
						_S.is_E = 1;
						break;
					case 'S':
						_S.is_S = 1;
						break;
					case 'v':
						printf("zcc version:\t%s\n", VERSION);
						break;
					case 'h':
						print_help();
						break;
					default:
						_S.errors++;
						break;
				}
			} else
				src_file_name = simple_string(argv[i]);
	}
	else
		_S.errors++;
	
	return;
}

/*===----------------------------------------------------------------------------------
calling my compiler, specifies all the filename and open the file pointer.
this is an horrible function maybe need split into two or three.
-----------------------------------------------------------------------------------===*/
void glue() {
	gas_file_head(asm_file, src_file_name);
	main_42();
	fclose(src_file);
	fclose(asm_file);

	return;
}

/*===--------------------------------------------------------------------------

---------------------------------------------------------------------------===*/
static void init() {
	init_files();
	init_lex();
	init_types();
	//init_tk2str();
	init_cnst_ast();
	init_regs();
	init_spill_loc();

	return;
}

/*===----------------------------------------------------------------------------------

----------------------------------------------------------------------------------===*/
int main(int argc, char* argv[]) {
	process_args(argc, argv);
	if (_S.errors) {
		if (!_S.is_H)
			print_help();
		return 1;
	}
	init();
	glue();

	return 0;
}
