#make file for running tests on frames
CC=gcc
CFLAGS=-I.

frametests:  frames.o utils.o frametests.o
	$(CC) -o frametests frames.o utils.o frametests.o

cleanframe:
	rm -f ./*.o ./frametests
