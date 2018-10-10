# CS454 Assignment 3

# Summary

Construct a multi-client, multi-server environment that relies on a binder to facilitate an RPC system.

## Signature

	Name: Jonathan Beverly
	Username: jrbeverl
	Student Id: 20418255

## Dependencies

In order to compile the applications and library, you will need to use the pthread library (lpthread) and have a C++11 compatible compiler.

 * pthread (lpthread)
 * C++11 (c++0x)

Compilation has been tested on the ubuntu student environments, specifically:

 * ubuntu1404-010

## Build Instructions

To make ("compile and link") all components of the project, you can quickly get started with

	make exec

Or if you are doing quick debugging

	make exec && ./binder

### RPC Lib

To make ("compile and link") the rpc library (librpc), use the included makefile or you can manually do it:

	g++ -std=c++0x -c rpcserver.cpp rpcclient.cpp client1.c server.c server_functions.c server_function_skels.c
	ar rcs librpc.a protocol.o rpcinfo.o helpers.o conversion.o bstream.o rpcserver.o rpcclient.o 

### Client

To make ("compile and link") the application, use the included makefile with the client directive specified.
For example, to make the client:

	make client

Or you can manually do it
        
    g++ -std=c++0x -L. client.o -lrpc -o client

Then, to run:

	./client

### Server

To make ("compile and link") the application, use the included makefile with the server directive specified.
For example, to make the server:

	make server

Or you can manually do it
        
    g++ -std=c++0x -L. server_functions.o server_function_skels.o server.o -lrpc -lpthread -o server

Then, to run:

	./server
