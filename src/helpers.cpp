#include "helpers.h"
#include "constants.h"
#include "rpc.h"
#include "conversion.h"

#include <netdb.h>
#include <limits.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

//--------------------------------------------------------------------------------------

// Logs the message to standard error.
void log_error(string msg, int code) {
    //std::cerr << msg << std::endl;
    exit(code);
}


//--------------------------------------------------------------------------------------
// Methods for addressing rpc argument bytes

short getArgTypeArrayLength(int argType) {
    // The argument type is defined as 4 bytes ([0][1][2][3]).
    // With it, we know that the last two bytes ([2][3]) contain the
    // length of the array (if the type is an array).  It is zero if the
    // type is a scalar
    return argType & 65535;
}

int getArgType(int argType) {
    // The argument type is defined as 4 bytes ([0][1][2][3]).
    // We know that the byte [1] contains the type definition.
    // So we shift 16-bits to remove the array length component, then
    // zero out the first [0] bit.
    return (argType >> 16) & 255;
}

unsigned int getArgTypesLength(int* argTypes) {
    // Increment until we find the zero arg
    unsigned int length = 0;
    while(*(argTypes + (length++)) != 0);
    return length;
}

bool isArgTypeInput(int argType) {
    // The argument type is defined as 4 bytes ([0][1][2][3]).
    // We are after the first bit of the [0] byte.
    return (argType >> 31) & 1;
}

bool isArgTypeOutput(int argType) {
    // The argument type is defined as 4 bytes ([0][1][2][3]).
    // We are after the second bit of the [0] byte.
    return ((argType & ( 1 << 30 )) >> 30) & 1;
}

//--------------------------------------------------------------------------------------

int type_sizeof(int type) {
    switch(type) {
        case ARG_CHAR:
            return sizeof(char);
            break;
        case ARG_SHORT:
            return sizeof(short);
            break;
        case ARG_INT:
            return sizeof(int);
            break;
        case ARG_LONG:
            return sizeof(long);
            break;
        case ARG_DOUBLE:
            return sizeof(double);
            break;
        case ARG_FLOAT:
            return sizeof(float);
            break;
        default:
            return -1;
    }
    return -1;
}

int socket_create(string server_identifier, int server_port) {
    // This code is to help opening of socket
    struct hostent *server;
    int socketfd;

    // Open socket on any details
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        return SOCKET_OPEN_ERROR;
    }

    server = gethostbyname(server_identifier.c_str());
    if (server == NULL) {
        return SOCKET_UNKNOWN_HOST;
    }

    // Create new socketaddr struct and wipe the data in it
    struct sockaddr_in serverAddress;
    memset((char*) &serverAddress, 0, sizeof(serverAddress));

    // Assignment to server address (port and server identifier)
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(server_port);
    bcopy((char*) server->h_addr, (char*) &serverAddress.sin_addr.s_addr, server->h_length);

    // Attempt to connect to the socket
    if (connect(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
        return SOCKET_CONNECTION_ERROR;
    }
    return socketfd;
}

int socket_listen(int socketfd) {
    // Create new socketaddr struct and wipe the data in it
    struct sockaddr_in binderAddress;
    memset((struct sockaddr_in*) &binderAddress, 0, sizeof(binderAddress));

    // Assignment 'any' host address information
    binderAddress.sin_family = AF_INET;
    binderAddress.sin_addr.s_addr = INADDR_ANY;
    binderAddress.sin_port = 0;

    // Attempt to bind the socket
    if (bind(socketfd, (struct sockaddr*) &binderAddress, sizeof(binderAddress)) < 0) {
        return SOCKET_BIND_ERROR;
    }

    // If successful, we set listening state
    listen(socketfd, SOMAXCONN);
    return 0;
}

int socket_accept(int socketfd) {
    // Create a new
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    int clientSocket = accept(socketfd, (struct sockaddr*) &clientAddress, &clientAddressSize);

    // Check if accepting of connection was successful
    if (clientSocket < 0) {
        return SOCKET_ACCEPT_ERROR;
    }
    return clientSocket;
}

string getHostname() {
    char localHostName[256];
    gethostname(localHostName, 256);
    return string(localHostName);
}

int getPort(int socketfd) {
    // get socket address
    struct sockaddr_in sin;
    socklen_t length = sizeof(sin);
    getsockname(socketfd, (struct sockaddr *)&sin, &length);

    // get port number
    int portno = ntohs(sin.sin_port);
    return portno;
}

string getBinderAddress() {
    char * address = getenv ("BINDER_ADDRESS");
    if(address == NULL) {
        log_error("FAILURE: BINDER_ADDRESS environment variable has not been set.", INIT_BINDER_ADDRESS_NOT_SET);
    }
    return address;
}

int getBinderPort() {
    char * portString = getenv("BINDER_PORT");
    if(portString == NULL) {
        log_error("FAILURE: BINDER_PORT environment variable has not been set.", INIT_BINDER_PORT_NOT_SET);
    }
    int port = atoi(portString);
    return port;
}

//return 1 if different. 0 if they are the same
int same_int_arr(int *arr1, int *arr2) {
    int count = 0;

    while (true) {
        int arg1 = getArgType(*(arr1 + count));
        int arg2 = getArgType(*(arr2 + count));
        if ((arg1  == 0) && (arg2 == 0)) {
            //means they are the same
            break;
        }
        else if (arg1 == 0) {
            return 1;
        }
        else if (arg2 == 0) {
            return 1;
        }
        else if (arg1 != arg2) {
            return 1;
        }
        else {
            count++;
        }
    }
    //reaches here is both are null terminiated
    return 0;
}

