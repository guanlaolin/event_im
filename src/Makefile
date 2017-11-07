CC=g++
FLAGS=-g -Wall
LIBS=-levent -lhiredis -lprotobuf
SRCS=main.cpp imredis.cpp immessage.pb.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET=im

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(FLAGS) $(LIBS)

%.o:%.cpp
	$(CC) $(FLAGS) $(LIBS) -o $@ -c $<

clean:
	rm -rf *.o $(TARGET) *.h.gch core.*