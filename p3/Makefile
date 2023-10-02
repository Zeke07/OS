CC = gcc
CFLAGS = -pedantic -std=gnu18
#-Wall -Werror
LOGIN = zkhan
SUBMITPATH = ~cs537-1/handin/$(LOGIN)/P3

.PHONY: all
all: wsh

wsh: wsh.c wsh.h
	$(CC) $(CFLAGS) -o $@ $<

run: wsh
	./wsh

pack: wsh.c wsh.h Makefile README.md
	tar -czvf $(LOGIN).tar.gz $^

submit: pack $(LOGIN).tar.gz
	cp $(LOGIN).tar.gz $(SUBMITPATH)

clean:
	rm -f wsh
	rm -f *.o