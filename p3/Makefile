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

tests:
	~cs537-1/tests/P3/test-pipe.csh -v
    #~cs537-1/tests/P3/test-cd.csh
    #~cs537-1/tests/P3/test-exec.csh
    #~cs537-1/tests/P3/test-job-control.csh


clean:
	rm -f wsh
	rm -f *.o