OBJECTS = chunk.o main.o memory.o debug.o value.o vm.o compiler.o scanner.o
CCFLAGS = -Wall
CC = gcc

all: $(OBJECTS)
	$(CC) -o clox $(OBJECTS)

chunk.o: chunk.h common.h

debug.o: debug.h common.h chunk.h

memory.o: memory.h common.h

value.o: value.h common.h

vm.o: vm.h common.h chunk.h value.h compiler.h

compiler.o: common.h compiler.h scanner.h

scanner.o: scanner.h common.h

main.o: chunk.h common.h vm.h

.PHONY : clean
clean:
	rm $(OBJECTS) 