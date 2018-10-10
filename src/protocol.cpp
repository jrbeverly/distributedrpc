#include "protocol.h"
#include "helpers.h"
#include "constants.h"
#include "conversion.h"
#include "bstream.h"
#include "rpc.h"

#include <cerrno>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>

using namespace std;

//--------------------------------------------------------------------------------------

// Creates an instance of the protocol controller.
Protocol::Protocol(int socketfd) {
    _sfd = socketfd;
}

//--------------------------------------------------------------------------------------

int Protocol::sendTerminate() {
    return sendMessage(0, TERMINATE, NULL);
}

//--------------------------------------------------------------------------------------

int Protocol::sendRegister(string server_identifier, unsigned short port, string name, int argTypes[]) {
    unsigned int count = getArgTypesLength(argTypes);

    // Allocate sending buffer and write format
    // Format is as follows: length of server identifier, server identifier, length of name, name,  argTypes array (ending in a zero)
    BinaryStream stream;
    stream.writeString(server_identifier);
    stream.writeInt16(port);
    stream.writeString(name);
    stream.writeUInt32(count);
    stream.writeInt32(argTypes, count);

    return sendMessage(stream.size(), REGISTER, stream.str());
}

int Protocol::sendRegisterResponse(ReasonCode code) {
    // Allocate a buffer for an 32-bit integer
    BinaryStream stream;
    stream.writeInt32(static_cast<int>(code));

    return sendMessage(stream.size(), REGISTER_SUCCESS, stream.str());
}

int Protocol::sendRegisterError(ReasonCode code) {
    // Allocate a buffer for an 32-bit integer
    BinaryStream stream;
    stream.writeInt32(static_cast<int>(code));

    // Sends the message
    return sendMessage(stream.size(), REGISTER_FAILURE, stream.str());
}

//--------------------------------------------------------------------------------------

int Protocol::sendLocationRequest(string name, int argTypes[]) {
    unsigned int count = getArgTypesLength(argTypes);

    // Allocate sending buffer and write format
    // Format is as follows: length of name, name, argTypes array (ending in a zero)
    BinaryStream stream;
    stream.writeString(name);
    stream.writeUInt32(count);
    stream.writeInt32(argTypes, count);

    // Sends the message
    return sendMessage(stream.size(), LOC_REQUEST, stream.str());
}

int Protocol::sendLocationResponse(string server_identifier, unsigned short port) {
    // Allocate sending buffer and write format
    // Format is as follows: length of server identifier, server identifier, port number
    BinaryStream stream;
    stream.writeString(server_identifier);
    stream.writeInt16(port);

    // Sends the message
    return sendMessage(stream.size(), LOC_SUCCESS, stream.str());
}

int Protocol::sendLocationError(ReasonCode reasonCode) {
    // Allocate a buffer for an 32-bit integer
    BinaryStream stream;
    stream.writeInt32(static_cast<int>(reasonCode));

    // Sends the message
    return sendMessage(stream.size(), LOC_FAILURE, stream.str());
}

//--------------------------------------------------------------------------------------

int Protocol::sendExecuteRequest(std::string name, int* argTypes, void**args) {
    unsigned int argTypesLength = getArgTypesLength(argTypes);

    // Write the initial format of the message to the stream
    // This format is:  { string length, the string, # of arguments, argument values}

    BinaryStream stream;
    stream.writeString(name);
    stream.writeUInt32(argTypesLength);
    stream.writeInt32(argTypes, argTypesLength);

    for(unsigned int i = 0; i < argTypesLength - 1; i++) {
        // The argument to write into the code
        int argType = argTypes[i];

        // Are we dealing with an array?
        void* argValue = args[i];
        int ctype = getArgType(argType);
        unsigned short length = getArgTypeArrayLength(argType);
        if(length == 0) {
            // It is a scalar
            length = 1;
        }

        // We loop around the number of values we need to write
        // If scalar, this loop happens only once
        switch(ctype) {
            case ARG_CHAR:
                stream.writeChar((char*) argValue, length);
                break;
            case ARG_SHORT:
                stream.writeInt16((short*) argValue, length);
                break;
            case ARG_INT:
                stream.writeInt32((int*) argValue, length);
                break;
            case ARG_LONG:
                stream.writeInt64((long*) argValue, length);
                break;
            case ARG_DOUBLE:
                stream.writeDouble((double*) argValue, length);
                break;
            case ARG_FLOAT:
                stream.writeFloat((float*) argValue, length);
                break;
            default:
                break;
        }
    }
    return sendMessage(stream.size(), EXECUTE, stream.str());
}

int Protocol::sendExecuteResponse(std::string name, int* argTypes, void**args) {
    unsigned int argTypesLength = getArgTypesLength(argTypes);

    // Write the initial format of the message to the stream
    // This format is:  { string length, the string, # of arguments, argument values}

    BinaryStream stream;
    stream.writeString(name);
    stream.writeInt32(argTypes, argTypesLength);

    for(unsigned int i = 0; i < argTypesLength - 1; i++) {
        // The argument to write into the code
        int argType = argTypes[i];

        // Are we dealing with an array?
        void* argValue = args[i];
        int ctype = getArgType(argType);
        unsigned short length = getArgTypeArrayLength(argType);
        if(length == 0) {
            // It is a scalar
            length = 1;
        }

        // We loop around the number of values we need to write
        // If scalar, this loop happens only once
        switch(ctype) {
            case ARG_CHAR:
                stream.writeChar((char*) argValue, length);
                break;
            case ARG_SHORT:
                stream.writeInt16((short*) argValue, length);
                break;
            case ARG_INT:
                stream.writeInt32((int*) argValue, length);
                break;
            case ARG_LONG:
                stream.writeInt64((long*) argValue, length);
                break;
            case ARG_DOUBLE:
                stream.writeDouble((double*) argValue, length);
                break;
            case ARG_FLOAT:
                stream.writeFloat((float*) argValue, length);
                break;
            default:
                break;
        }
    }

    return sendMessage(stream.size(), EXECUTE_SUCCESS, stream.str());
}

int Protocol::sendExecuteError(ReasonCode reasonCode) {
    // Allocate a buffer for an 32-bit integer
    BinaryStream stream;
    stream.writeInt32(static_cast<int>(reasonCode));

    // Sends the message
    return sendMessage(stream.size(), EXECUTE_FAILURE, stream.str());
}

//--------------------------------------------------------------------------------------

int Protocol::sendLocationCacheRequest(string name, int*argTypes) {
    unsigned int count = getArgTypesLength(argTypes);

    // Allocate sending buffer and write format
    // Format is as follows: length of name, name, argTypes array (ending in a zero)
    BinaryStream stream;
    stream.writeString(name);
    stream.writeUInt32(count);
    stream.writeInt32(argTypes, count);

    // send message
    return sendMessage(stream.size(), LOC_CACHE_REQUEST, stream.str());
}

int Protocol::sendLocationCacheResponse(list<function_info*> &services) {
    // Allocate sending stream
    BinaryStream stream;
    stream.writeUInt32(services.size());
    for(auto const service : services) {
        // The format is as follows: length of server_identifier, server_identifier, port
        stream.writeString(service->server_identifier);
        stream.writeInt16(service->port);
    }

    return sendMessage(stream.size(), LOC_CACHE_SUCCESS, stream.str());
}

int Protocol::sendLocationCacheError(ReasonCode reasonCode) {
    // Allocate a buffer for an 32-bit integer
    BinaryStream stream;
    stream.writeInt32(static_cast<int>(reasonCode));

    // Sends the message
    return sendMessage(stream.size(), LOC_CACHE_FAILURE, stream.str());
}

//--------------------------------------------------------------------------------------

int Protocol::receiveMessageType(MessageType &type) {
    // Allocate a buffer for an 32-bit integer
    char buffer[SIZEOF_INTEGER];

    // receive the message (if zero then success)
    int status = receiveMessage(SIZEOF_INTEGER, buffer);
    if (status != 0) {
        return status;
    }

    // Read the integer from the byte stream
    int result = Convert::parseInt32(buffer);
    type = static_cast<MessageType>(result);
    return 0;
}

int Protocol::receiveMessageSize(unsigned int &messageSize) {
    // Allocate a buffer for an 32-bit integer
    char buffer[SIZEOF_INTEGER];

    // receive the message (if zero then success)
    int status = receiveMessage(SIZEOF_INTEGER, buffer);
    if(status != 0) {
        return status;
    }

    // Read the integer from byte stream
    messageSize = Convert::parseUInt32(buffer);
    return 0;
}

int Protocol::receiveMessage(unsigned int size, char message[]) {
    //NOTE: This would be better as vector<char> and populating that instead
    // If empty, return
    if (size == 0) {
        return 0;
    }

    int bytesRead;
    unsigned int bytesRemaining = size;

    // Continue reading the files
    while (bytesRemaining > 0) {
        memset(message, 0, bytesRemaining);

        // Read bytes into the buffer
        bytesRead = recv(_sfd, message, bytesRemaining, 0);
        if(bytesRead == 0) {
            return SOCKET_CONNECTION_ERROR;
        }
        else if (bytesRead < 0) {
            return SOCKET_RECEIVE_ERROR;
        }

        // Adjusts pointer and remaining bytes
        message += bytesRead;
        bytesRemaining -= bytesRead;
    }

    return 0;
}

//--------------------------------------------------------------------------------------

int Protocol::sendMessage(unsigned int messageSize, MessageType messageType, char message[]) {
    // Allocate the buffer, and send the contents
    BinaryStream stream;
    stream.writeUInt32(messageSize);
    stream.writeInt32(static_cast<int>(messageType));
    stream.writeChar(message, messageSize);

    // Gets the buffer pointer from the stream
    char* pointer = stream.str();
    unsigned int bytesRemaining = stream.size();
    int bytesRead;

    while (bytesRemaining > 0) {
        // Read from the socket the number of bytes specified
        bytesRead = send(_sfd, pointer, bytesRemaining, 0);

        if (bytesRead == 0) {
            break;
        }
        else if (bytesRead < 0) {
            // if negative, then errwor
            return bytesRead;
        }

        // Update number of bytes left to read (+ adjust pointer)
        bytesRemaining -= bytesRead;
        pointer += bytesRead;
    }

    // Return bytesRemaining (which should be 0)
    return bytesRemaining;
}