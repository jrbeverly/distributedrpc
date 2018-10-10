CXX=g++
CXXFLAGS=-g -std=c++0x -w

OBJECTS1 = binder.o protocol.o helpers.o rpcinfo.o conversion.o bstream.o
EXEC1 = binder

OBJECTS = ${OBJECTS1}
EXECS = ${EXEC1}

#############################################################

all : ${EXECS}
	$(CXX) $(CXXFLAGS) -c rpcserver.cpp rpcclient.cpp server.c client1.c server_functions.c server_function_skels.c
	ar rcs librpc.a protocol.o rpcinfo.o helpers.o conversion.o rpcserver.o rpcclient.o bstream.o

client: all
	$(CXX) $(CXXFLAGS) -L. client1.o -lrpc -o client

server: all
	$(CXX) $(CXXFLAGS) -L. server_functions.o server_function_skels.o server.o -lrpc -lpthread -o server

exec: clean client server
	$(CXX) $(CXXFLAGS) -L. client1.o -lrpc -o client
	$(CXX) $(CXXFLAGS) -L. server_functions.o server_function_skels.o server.o -lrpc -lpthread -o server
	mkdir -p ../out
	mv client server binder ../out/
	$(MAKE) clean

${EXEC1} : ${OBJECTS1}
	$(CXX) $^ -o $@ 

#############################################################

clean :
	rm -f *.d *.o *.a ${EXECS} client server