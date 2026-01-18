huncolor: huncolor.o
huncolor: LDLIBS=-lhunspell-1.7
huncolor.o: huncolor.c

install: huncolor
	install huncolor $(HOME)/bin
