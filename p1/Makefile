
build: wman.c wapropos.c
	gcc wman.c -Wall -Werror -o wman
	gcc wapropos.c -Wall -Werror -o wapropos
	gcc wgroff.c -Wall -Werror -o wgroff

memleak: wman
	valgrind --leak-check=yes --log-file=wman.rpt wman 2 wman
	valgrind --leak-check=yes --log-file=wman2.rpt wman wballs
	valgrind --leak-check=yes --log-file=wapropos.rpt wapropos kotakota
	valgrind --leak-check=yes --log-file=wgroff.rpt wgroff input_files/wgroff.txt
	valgrind --leak-check=yes --log-file=wgroff2.rpt wgroff input_files/bad_header_date.txt
	valgrind --leak-check=yes --log-file=wgroff3.rpt wgroff input_files/example.txt
	valgrind --leak-check=yes --log-file=wgroff4.rpt wgroff input_files/bad_header_section.txt
	valgrind --leak-check=yes --log-file=wgroff5.rpt wgroff input_files/wman.txt



clean:
	rm wman
	rm wapropos
	rm wgroff
	rm *.rpt
	rm *.1
	rm *.2
	rm *.txt

