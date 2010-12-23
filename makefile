all:
	gcc -g -Wall *.c
opt1:
	gcc -g -Wall -O1 *.c
opt2:
	gcc -g -Wall -O2 *.c
opt3:
	gcc -g -Wall -O3 *.c
