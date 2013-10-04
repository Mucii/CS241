# CS 241
# University of Illinois

CC = gcc
INC = -I../mp7/libds -I../mp8/libs -I../mpx_client/libjson
FLAGS = -g -W -Wall
LIBS = -L../mpx_client/libjson -lpthread -ljson


all: ../mpx_client/libjson/libjson.a server


OBJECTS = libds.o libhttp.o libdictionary.o 

libds.o: ../mp7/libds/libds.c ../mp7/libds/libds.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

libhttp.o: ../mp8/libs/libhttp.c ../mp8/libs/libhttp.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

libdictionary.o: ../mp8/libs/libdictionary.c ../mp8/libs/libdictionary.h
	$(CC) -c $(FLAGS) $(INC) $< -o $@ $(LIBS)

../mpx_client/libjson/libjson.a:
	$(MAKE) -C ../mpx_client/libjson


server: $(OBJECTS) server.c
	$(CC) $(FLAGS) $(INC) $^ -o $@ $(LIBS)

clean:
	rm -rf server *.o