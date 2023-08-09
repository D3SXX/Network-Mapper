OBJS	= main.o
SOURCE	= main.c
HEADER	= 
OUT	= NetworkMapper
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 =
LIBS	 = `pkg-config --libs gtk+-3.0`
CFLAGS	 = `pkg-config --cflags gtk+-3.0`

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LIBS) $(LFLAGS)

main.o: main.c
	$(CC) $(FLAGS) $(CFLAGS) main.c 

clean:
	rm -f $(OBJS) $(OUT)
