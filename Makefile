SRCMODULES=base.c log.c routes.c host.c http.c string_utils.c set.c user.c
OBJMODULES=$(SRCMODULES:.c=.o)
CC = gcc
CFLAGS = -Wall -g -ansi -pedantic

VPATH := . core utils user

lyter: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -c -o $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRCMODULES)
	$(CC) -MM $^ > $@
	

clean:
	rm -f *.o lyter deps.mk

