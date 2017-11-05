CC=g++
FLAGS=-g -Wall
LIBS=-levent -lhiredis
SRC=redis.c redis.h
PROG=im

im_redis.o:
	$(CC) $(FLAGS) -c im_redis.cpp im_redis.h

im:
	$(CC) $(FLAGS) $(LIBS) main.c im_redis.o -o $(PROG)

clean:
	rm -rf *.o $(PROG)