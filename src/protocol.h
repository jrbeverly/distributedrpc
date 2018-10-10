#pragma once

#include "rpcinfo.h"
#include "constants.h"
#include "conversion.h"
#include "bstream.h"

#include <string>
#include <list>


// Macros to remove magic numbers
#define SIZEOF_LENGTH 4
#define SIZEOF_TYPE 4
#define SIZEOF_ARGYPE 4
#define SIZEOF_INTEGER 4
#define SIZEOF_SHORT 2
#define SIZEOF_PORT 2
#define SIZEOF_NULLTERM 1

//--------------------------------------------------------------------------------------

class Protocol {
  public:
    // Creates an instance of the protocol controller.
    Protocol(int socketfd);

    //--------------------------------------------------------------------------------------
    // Methods that define the protocol messages.

    // Sends a termination message.
    int sendTerminate();

    // Sends the register request with the socket host address and remote procedure command (rpc) definition.
    int sendRegister(std::string server_identifier, unsigned short port, std::string name, int argTypes[]);

    // Sends the register success response with any possible warnings or errors.
    int sendRegisterResponse(ReasonCode code);

    // Sends the register error response with the associated error.
    int sendRegisterError(ReasonCode code);

    // Sends the cached location request with the remote procedure command (rpc) definition.
    int sendLocationCacheRequest(std::string name, int*argTypes);

    // Sends the cached location success response with the specified server identifier and port
    int sendLocationCacheResponse(std::list<function_info*> &services);

    // Sends the cached location error response with the specified reasonCode
    int sendLocationCacheError(ReasonCode reasonCode);

    // Sends the location request with the remote procedure command (rpc) definition.
    int sendLocationRequest(std::string name, int argTypes[]);

    // Sends the location success response with the specified server identifier and port
    int sendLocationResponse(std::string server_identifier, unsigned short port);

    // Sends the location error response with the specified reasonCode
    int sendLocationError(ReasonCode reasonCode);

    // Sends the execute request with the function and parameters.
    int sendExecuteRequest(std::string name, int* argTypes, void**args);

    // Sends the execute response with the function and parameters.
    int sendExecuteResponse(std::string name, int* argTypes, void**args);

    // Sends the execute error response with the specified reasonCode.
    int sendExecuteError(ReasonCode reasonCode);

    //--------------------------------------------------------------------------------------
    // Methods that handle receiving of data from bound socket.

    // Receives the message contents from the bound socket.
    int receiveMessage(unsigned int size, char message[]);

    // Receives the message size from the bound socket.
    int receiveMessageSize(unsigned int &messageSize);

    // Receives the message type from the bound socket.
    int receiveMessageType(MessageType &type);

  private:

    // Sends the structural protocol message over the currently bound socket.
    int sendMessage(unsigned int messageSize, MessageType msgType, char message[]);

    int _sfd;
};