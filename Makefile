SRCMODULES=base.c log.c routes.c http.c string_utils.c set.c user.c
OBJMODULES=$(SRCMODULES:.c=.o)
CC = gcc
CFLAGS = -Wall -g -ansi -pedantic

VPATH := . core utils user

server: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -c -o $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRCMODULES)
	$(CC) -MM $^ > $@
	

clean:
	rm -f *.o server

