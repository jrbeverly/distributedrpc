#pragma once

/*
 * rpcinfo.h
 *
 * This file defines common types used by the application
 */

#include <string>

//--------------------------------------------------------------------------------------
// Provides a container class for socket host address information.
struct server_info {

    // Gets the host name of the local computer.
    std::string server_identifier;

    // Gets the port number of the socket file descriptor.
    unsigned short port;

    // Creates an instance of the server_info class with the specified server_identifier and port
    server_info(std::string server_identifier, unsigned short port)
        : server_identifier(server_identifier), port(port) {}

};

//--------------------------------------------------------------------------------------
// Provides a container class for a server that supports a remote procedure command definition.
struct function_info {

    // Gets the host name of the local computer.
    std::string server_identifier;

    // Gets the port number of the socket file descriptor.
    unsigned short port;

    // The remote protocol command (RPC) definition
    struct rpc_info *rpcdef;

    // Creates an instance of the function_info class with the specified server_identifier, port and rpc function
    function_info(std::string server_identifier, unsigned short port,  struct rpc_info *rpcdef)
        : server_identifier(server_identifier), port(port), rpcdef(rpcdef) {}
};

//--------------------------------------------------------------------------------------
// Provides a container class for a remote procedure command definition.
struct rpc_info {

    // Gets the name of the remote protocol command (RPC).
    std::string name;

    // Gets the parameter types of this command.
    int *argTypes;

    // Creates an instance of the rpc_info class with specified rpc definition.
    rpc_info(std::string name, int *argTypes)
        : name(name), argTypes(argTypes) {}
};

//--------------------------------------------------------------------------------------
// Boolean operators for operations
bool operator ==(const server_info &l, const function_info &r);
bool operator ==(const server_info &l, const server_info &r);
bool operator ==(const function_info &l, const function_info &r);

// Needed for map
bool operator <(const rpc_info &l, const rpc_info &r);