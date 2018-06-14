C=clang
CFLAGS= -std=c99 -Wall -Werror -pedantic -g
DEPS=header.h

all:
	$(C) $(CFLAGS) *.c -L./lib64/ -lruntime

clean:
	rm -f *.o *~ core
	rm *.out
	rm *.log
tar:
	tar czf ../pa2.tar.gz  --exclude='pa2.pdf' --exclude='a.out' --exclude="*.un~" --exclude="*.log" --exclude="script.sh" --exclude='tags' --exclude="pa2.pro*" ../pa2
