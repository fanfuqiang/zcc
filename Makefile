OBJS	=	./src/driver.o\
		./src/alloc.o\
		./src/decl.o\
		./src/error.o\
		./src/expr.o\
		./src/gas.o\
		./src/icode.o\
		./src/infer.o\
		./src/lex.o\
		./src/opt.o\
		./src/reg.o\
		./src/stmt.o\
		./src/string.o\
		./src/symbol.o\
		./src/type.o\
		./src/utils.o\
		./src/zbuf.o

CC		= gcc

all: $(OBJS)
	$(CC) -o zcc -g $(OBJS)

$(OBJS): %.o: %.c
	$(CC) -c -g $< -o $@

clean:
	rm -f $(OBJS)
