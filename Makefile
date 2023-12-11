CC = gcc
CFLAGS = -lm

target: buffer.o hashtable.o linkedlist.o
	$(CC) buffer.o hashtable.o linkedlist.o -o buffer -lm

buffer.o: buffer.c
	$(CC) -c buffer.c -lm

hashtable.o: ./DataStructureLibrary/hashtable.c ./DataStructureLibrary/hashtable.h
	$(CC) -c DataStructureLibrary/hashtable.c

linkedlist.o: ./DataStructureLibrary/linkedlist.c ./DataStructureLibrary/linkedlist.h
	$(CC) -c DataStructureLibrary/linkedlist.c

clean:
	rm *.o

