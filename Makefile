CC=gcc
FLAGS=-g -Wall
LIBS=-levent
PROG=im

im:
	$(CC) $(FLAGS) $(LIBS) main.c -o $(PROG)

clean:
	rm -rf *.o $(PROG)