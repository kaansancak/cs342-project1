all: prime	mqprime
prime:	prime.c
	gcc -Wall -g -o prime prime.c
mqprime: mqprime.c
	gcc -Wall -g -o mqprime mqprime.c -lrt