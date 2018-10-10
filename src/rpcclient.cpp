#include "rpc.h"
#include "helpers.h"
#include "protocol.h"
#include "constants.h"
#include "conversion.h"
#include "bstream.h"
#include "rpcinfo.h"

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

using namespace std;

// A cached list of services similar to the one known by the binder.
static map<rpc_info, list<function_info>> m_serviceMap;

// The connection socket to the binder.
static int m_binderSocket = -1;

//--------------------------------------------------------------------------------------

// Establishes a connection with the binder.
int binder_connect() {
    // If we already have a connection, do not bother
    if (m_binderSocket >= 0) {
        return 0;
    }

    // Get socket address information
    string binderAddress = getBinderAddress();
    int binderPort = getBinderPort();

    // Open socket
    m_binderSocket = socket_create(binderAddress, binderPort);

    // If less than zero, we have binder socket failure
    if (m_binderSocket < 0) {
        return INIT_BINDER_SOCKET_ERROR;
    }

    // otherwise good.
    return 0;
}

//--------------------------------------------------------------------------------------

// Processes the execute response by reaidng the values into the buffer
int processExecuteResponse(BinaryStream stream, int argTypes[], void * args[], unsigned int argTypesLength) {
    stream.readInt32(argTypes, argTypesLength);
    // This read code is based on protocol.h / sendExecuteResponse

    // Iterate through the arguments
    for(unsigned int i = 0; i < argTypesLength - 1; i++) {
        int argType = argTypes[i];

        // if it is an output argument, then we do not need
        // to send this back (empty stub is fine)
        if (!isArgTypeOutput(argType)) {
            continue;
        }

        int type = getArgType(argType);
        short length = getArgTypeArrayLength(argType);

        // We are working with a scalar, so we resolve it to a value in the array
        if (length == 0) {
            switch(type) {
                case ARG_CHAR:
                    *((char*)args[i]) = stream.readChar();
                    break;
                case ARG_SHORT:
                    *((short*)args[i]) = stream.readInt16();
                    break;
                case ARG_INT:
                    *((int*)args[i]) = stream.readInt32();
                    break;
                case ARG_LONG:
                    *((long*)args[i]) = stream.readInt64();
                    break;
                case ARG_DOUBLE:
                    *((double*)args[i]) = stream.readDouble();
                    break;
                case ARG_FLOAT:
                    *((float*)args[i]) = stream.readFloat();
                    break;
                default:
                    break;
            }
        }
        else {
            // We are working with an array, so read into the array object (based on length) for the execute parameters
            switch(type) {
                case ARG_CHAR:
                    stream.readChar((char*)args[i], length);
                    break;
                case ARG_SHORT:
                    stream.readInt16((short*)args[i], length);
                    break;
                case ARG_INT:
                    stream.readInt32((int*)args[i], length);
                    break;
                case ARG_LONG:
                    stream.readInt64((long*)args[i], length);
                    break;
                case ARG_DOUBLE:
                    stream.readDouble((double*)args[i], length);
                    break;
                case ARG_FLOAT:
                    stream.readFloat((float*)args[i], length);
                    break;
                default:
                    break;
            }
        }
    }
    return 0;
}

// Sends an execute request to the server
int sendExecuteRequest(int socketfd, char* name, int* argTypes, void** args) {
    Protocol handler(socketfd);
    int status = 0;

    // Send the execute command
    status = handler.sendExecuteRequest(name, argTypes, args);
    if (status != 0) {
        return status;
    }

    // -----------------------
    // After sending the execute, we are now waiting for a respond from the server
    // The response will be of the form:
    // Length, Type, Message (contents)
    unsigned int length;
    MessageType type;

    // Get the length of the message.
    status = handler.receiveMessageSize(length);
    if (status < 0) {
        return status;
    }

    // Get the type of the message.
    status = handler.receiveMessageType(type);
    if (status < 0) {
        return status;
    }

    // Using the length, we allocate a buffer for the reply contents
    BinaryStream stream(length);

    // Get the reply contents
    status = handler.receiveMessage(length, stream.str());
    if (status < 0) {
        return status;
    }

    // If type is failure, then we need to exit
    if (type == EXECUTE_FAILURE) {
        int returnCode = stream.readInt32();
        return returnCode;
    }

    // We are expecting a success response, if we do not get it
    // then the message type is invalid
    if (type != EXECUTE_SUCCESS) {
        return RECEIVE_INVALID_MESSAGE_TYPE;
    }

    // Get number of arguments & compute length
    unsigned int argCount = getArgTypesLength(argTypes);
    string functionName = stream.readString();

    if(strcmp(functionName.c_str(), name) == 0) {
        processExecuteResponse(stream, argTypes, args, argCount);
    }
    else {
        return RECEIVE_INVALID_COMMAND_NAME;
    }

    return 0;
}

// Send an execute request to the next most available server in the list.
int sendExecuteToAvailable(char * name, int*argTypes, void**args, list<function_info> &services) {
    int status = 0;

    // We need to find a server from the list to send execute to, so send it.
    for (function_info service : services) {

        // Open socket with the service, if fails, move to the next one
        int socketfd = socket_create(service.server_identifier, service.port);
        if(socketfd < 0) {
            continue;
        }

        // We opened a connection, now send the execute request.
        status = sendExecuteRequest(socketfd, name, argTypes, args);

        // Close the socket as we are done with this regardless of success
        close(socketfd);

        // if success, finish, if not successful continue to next server
        if (status == 0) {
            return 0;
        }
    }

    return -1;
}

//--------------------------------------------------------------------------------------

// Process a location response returned from the server
int processLocationResponse(string &server_identifier, unsigned short &port) {
    Protocol handler(m_binderSocket);
    int status = 0;

    // We are receiving a message of the format
    // LENGTH, TYPE, MESSAGE (contents)
    MessageType msgType;
    unsigned int messageSize;

    // Get the length of the message
    status = handler.receiveMessageSize(messageSize);
    if(status < 0) {
        return status;
    }

    // Get the type of the message
    status = handler.receiveMessageType(msgType);
    if(status < 0) {
        return status;
    }

    // Allocate a buffer to store the message.
    BinaryStream stream(messageSize);

    // Receive the message contents
    status = handler.receiveMessage(messageSize, stream.str());
    if(status < 0) {
        return status;
    }

    // if failure, get reasonCode and exit
    if (msgType == LOC_FAILURE) {
        int reasonCode = stream.readInt32();
        return reasonCode;
    }

    // We expect a SUCCESS message, if it is not this, then
    // we have an invalid message type
    if (msgType != LOC_SUCCESS) {
        return RECEIVE_INVALID_MESSAGE_TYPE;
    }

    // Parse the message
    server_identifier = stream.readString();
    port = stream.readUInt16();

    return 0;
}

// Performs a call of an remote procedure command with the specified arguments.
int rpcCall(char* name, int* argTypes, void** args) {
    int status = 0;

    // Send location request, if error, then exit
    string command_name(name);

    // Open connection to binder
    status = binder_connect();

    // If error, return error code
    if (status != 0) {
        return status;
    }

    // Open protocol handler and send location request with args
    Protocol handler(m_binderSocket);
    status = handler.sendLocationRequest(name, argTypes);
    if(status < 0) {
        return status;
    }

    string server_identifier;
    unsigned short port;

    // Process the incoming location response with server id and port
    status = processLocationResponse(server_identifier, port);
    if (status < 0) {
        return status;
    }

    {
        // Open a new socket connection with the server
        int serverSocket = socket_create(server_identifier, port);

        // Send command to the server
        status = sendExecuteRequest(serverSocket, name, argTypes, args);

        // cleanup
        close(serverSocket);
    }

    return status;
}

//--------------------------------------------------------------------------------------

// Handle a location cache call
int processLocationCacheCall(BinaryStream& stream, rpc_info &command) {
    unsigned int count = stream.readUInt32();

    list<function_info> list;
    for (int i = 0; i < count; i++) {
        string server_identifier = stream.readString();
        unsigned short port = stream.readUInt16();

        // Add newly discovered supported server to the system
        function_info service(server_identifier, port, &command);
        list.push_back(service);
    }

    m_serviceMap[command] = list;
    return 0;
}

// Performs a call of an remote procedure command with the specified arguments.
// This command looks for previously known servers to perform the connection.
int rpcCacheCall(char* name, int* argTypes, void** args) {
    string command_name(name);
    int status = 0;

    // if it already exists in cache, use cache
    rpc_info command(command_name, argTypes);
    if(m_serviceMap.find(command) != m_serviceMap.end()) {
        list<function_info> service = m_serviceMap[command];
        status = sendExecuteToAvailable(name, argTypes, args, service);

        if (status == 0) {
            return 0;
        }
    }
    m_serviceMap.erase(command);

    // else fetch new servers from binder
    // send request

    status = binder_connect();
    if (status != 0) {
        return status;
    }

    // Use protocol handler to send a request for the location cache data
    // based on the current rpc information
    Protocol handler(m_binderSocket);
    handler.sendLocationCacheRequest(command_name, argTypes);

    // -----------------------
    // After sending the execute, we are now waiting for a response from the server
    // The response will be of the form:
    // Length, Type, Message (contents)
    unsigned int length;
    MessageType type;

    // Get the length of the message.
    status = handler.receiveMessageSize(length);
    if(status < 0) {
        return status;
    }

    // Get the type of the message.
    status = handler.receiveMessageType(type);
    if(status < 0) {
        return status;
    }

    // If the length is zero, that is invalid.  We need to receive some number of
    // locations for this to work, and if the binder does not provide, we have a problem
    if(length == 0) {
        return RECEIVE_INVALID_MESSAGE;
    }

    // Using the length, we allocate a buffer for the reply contents
    char buffer[length];
    status = handler.receiveMessage(length, buffer);
    if(status < 0) {
        return status;
    }

    // Create binary stream handler
    BinaryStream stream(buffer, length);

    // If failure, get error code and return
    if (type == LOC_CACHE_FAILURE) {
        int errorCode = stream.readInt32();
        return errorCode;
    }

    // If response is not a cache success, it is invalid
    if(type != LOC_CACHE_SUCCESS) {
        return RECEIVE_INVALID_MESSAGE_TYPE;
    }

    // Process the response from the location cache
    status = processLocationCacheCall(stream, command);
    if(status < 0) {
        return status;
    }

    // Search the newly updated cache for the function.  If it does not exist
    // Then function is not available (404)
    if (m_serviceMap.find(command) == m_serviceMap.end()) {
        return FUNCTION_NOT_AVAILABLE;
    }

    // Function exists, get list of services and try to execute the command on once of them
    list<function_info> services = m_serviceMap[command];
    status = sendExecuteToAvailable(name, argTypes, args, services);

    return status;
}

//--------------------------------------------------------------------------------------

// Send an termination request to the binder.
int rpcTerminate() {
    int status = 0;

    // Are we connected? if not, connect so we can tell it to shutdown
    if (m_binderSocket < 0) {
        status = binder_connect();
    }

    // Failure status while connecting to binder, return status
    // It is possible that binder has already shutdown
    if (status != 0) {
        return status;
    }

    // Open the protocol so we can tell binder to terminate
    Protocol handler(m_binderSocket);
    status = handler.sendTerminate();

    return status;
}