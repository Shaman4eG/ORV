C=gcc
CFLAGS= -std=c99 -Wall -Werror -g
DEPS=header.h

all:ipc

ipc:parent.o child.o communicate.o
	$(C) -o ipc parent.o child.o communicate.o
parent:parent.c $(DEPS)
	$(C) $(CFLAGS) -c parent.c
child:child.c $(DEPS)
	$(C) $(CFLAGS) -c child.c
communicate:communicate.c $(DEPS)
	$(C) $(CFLAGS) -c communicate.c
cleanO:
	rm -rf *.o
clean:
	rm -f *.o *~ core 
