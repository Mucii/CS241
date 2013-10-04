# CS 241
# University of Illinois

CC = gcc
INC = -I. -I../mp8/libs -Ilibjson
FLAGS = -g -W -Wall
LIBS = -Llibjson -lpthread -ljson

OBJECTS = libmapreduce.o libds.o libhttp.o libdictionary.o

all: libjson/libjson.a test1 test2


#
# Objects / Libraries
#
libjson/libjson.a:
	$(MAKE) -C libjson

libds.o: libds/libds.c libds/libds.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

libhttp.o: ../mp8/libs/libhttp.c ../mp8/libs/libhttp.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

libdictionary.o: ../mp8/libs/libdictionary.c ../mp8/libs/libdictionary.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

libmapreduce.o: libmapreduce.c libmapreduce.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)


#
# Programs
#
test1: $(OBJECTS) test1.c
	$(CC) $(FLAGS) $^ -o $@ $(LIBS)

test2: $(OBJECTS) test2.c
	$(CC) $(FLAGS) $^ -o $@ $(LIBS)

clean:
	rm -rf *.o *.d test1 test2 test3 test4 test5 test6 doc/html *~
