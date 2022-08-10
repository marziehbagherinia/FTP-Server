CC = g++
CXXFLAGS += -std=c++11 -w -O3

all: ftp ftp-server
.PHONY: clean

client = client/main.o client/client.o includes/util.o
server = server/main.o server/server.o server/handler.o includes/util.o

ftp: $(client)
	$(CC) -o $@ $^
ftp-server: $(server)
	$(CC) -pthread -o $@ $^


client/main.o: client/main.cpp
client/client.o: client/client.h
server/main.o: server/main.cpp
server/server.o: server/server.h
server/handler.o: server/handler.h
includes/util.o: includes/util.h

clean:
	rm client/*.o
	rm server/*.o
	rm includes/*o
	rm ftp
	rm ftp-server
