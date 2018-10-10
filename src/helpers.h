#pragma once

#include <string>
#include "constants.h"
#include "conversion.h"

//--------------------------------------------------------------------------------------
// Logging methods

//TODO: Move to a logger

// Logs the message to standard error.
void log_error(std::string msg, int code);

//--------------------------------------------------------------------------------------
// Methods for addressing rpc argument bytes

// Returns the length of the argument if the type is an array; otherwise if it is a scalar, returns zero.
short getArgTypeArrayLength(int argType);

// Returns the standard C type of the rpc argument.
int getArgType(int argType);

// Returns the number of arguments in the argTypes array.
unsigned int getArgTypesLength(int * array);

// Determines if the argument is an input.
bool isArgTypeInput(int argType);

// Determines if the argument is an input.
bool isArgTypeOutput(int argType);

//--------------------------------------------------------------------------------------
// Code for handling type data

// Returns the size in bytes of the type defined by the specfied type identifier.
int type_sizeof(int type);

//--------------------------------------------------------------------------------------
// Code for handling socket host address information

// Returns the current hostname
std::string getHostname();

// Returns the current port number of the socket
int getPort(int socketfd);

// Returns the binder address configuration value
std::string getBinderAddress();

// Returns the binder port configuration value
int getBinderPort();

//--------------------------------------------------------------------------------------
// Code for common socket behaviour

//TODO: Move this to a transmitter class (allows stubbing of the code)

//creates and bind a new socket
//exactly like socket_create but with a bind before connect
int socket_create_new(std::string server_identifier);

// Opens a socket at the specified hostname and port number.
int socket_create(std::string server_identifier, int port);

// Places a socket in a listening state.
int socket_listen(int socketfd);

// Creates a new socket for a newly created connection.
int socket_accept(int socketfd);


//---------------------------------------------------------------------------------------
//useful for server and possibly for binder
int same_int_arr(int *arr1, int *arr2);
