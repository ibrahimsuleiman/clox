OBJECTS = chunk.o main.o memory.o debug.o value.o
CCFLAGS = -Wall
CC = gcc

all: $(OBJECTS)
	$(CC) -o clox $(OBJECTS)

chunk.o: chunk.h common.h

debug.o: debug.h common.h chunk.h

memory.o: memory.h common.h

value.o: value.h common.h

main.o: chunk.h common.h

.PHONY : clean
clean:
	rm $(OBJECTS) clox