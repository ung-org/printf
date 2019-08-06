.POSIX:

all: echo printf

printf: printf.c

echo: printf
	ln -s printf $@

clean:
	rm -f *.o echo printf
