OBJS	= driver.o alloc.o decl.o error.o expr.o gas.o icode.o\
			infer.o lex.o opt.o reg.o stmt.o string.o symbol.o\
			type.o utils.o zbuf.o

CC		= gcc

all: $(OBJS)
	$(CC) -o zcc -g $(OBJS)

$(OBJS): %.o: %.c
	$(CC) -c -g $< -o $@

clean:
	rm -f *.o
