#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <list>
#include <strings.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <sstream>
#include <signal.h>
#include <map>
#include <unistd.h>
#include <cstdint>

#include "protocol.h"
#include "helpers.h"
#include "constants.h"
#include "rpcinfo.h"
#include "rpc.h"
#include "conversion.h"
#include "bstream.h"

using namespace std;

// Determines if binder is running
static bool m_running;

// The map of rpc_functions to known server functions
static map<rpc_info, list<function_info*>*> m_serverFunctionMap;

// The server priority queue
static list<server_info*> m_priorityQueue;

// Socket specific details such as server information and currently available bytes
static map<int, server_info *> m_socketServerMap;
static map<int, unsigned int> m_messageBlocks;
static map<int, MessageType> m_messageInfo;

// Gets the command supported server with the highest priority for the command.
server_info *getPriorityServer(const rpc_info& command) {
    // We need to get the server that is msot recent in the queue
    // that supports the current RPC, so we iterate through
    // priority queue looking for a match and pushing encountered
    // servers to back of queue.

    // Check if we have any servers related to the current rpc function
    if (m_serverFunctionMap.find(command) == m_serverFunctionMap.end()) {
        return NULL;
    }

    // Get the servers that support this remote procedure command
    list<function_info*> *supported_servers = m_serverFunctionMap[command];
    server_info *rpc_server = NULL;

    // Iterate through the priority queue
    for (unsigned int i = 0; i < m_priorityQueue.size(); i++) {
        // Get the known server
        server_info *server = m_priorityQueue.front();

        // Iterate through the servers finding the first matching one
        for (auto const &supportingServer : *supported_servers) {
            // If they have same server, then ok good
            if (*server == *supportingServer) {
                rpc_server = server;
                break;
            }
        }

        // Performs a queue 'roll', cycling the queue
        m_priorityQueue.splice(m_priorityQueue.end(), m_priorityQueue, m_priorityQueue.begin());

        // If not null then leave priority queue
        if (rpc_server != NULL) {
            break;
        }
    }

    return rpc_server;
}

// Adds a supported remote procedure command for the specified server
ReasonCode function_add(string name, int argTypes[], string server_identifier, unsigned short port) {
    // Constructs the server and command
    server_info location(server_identifier, port);
    rpc_info command(name, argTypes);
    ReasonCode result = SUCCESS;

    // We are adding the function to the map, we need to convert from stack to heap
    unsigned int length = getArgTypesLength(argTypes);
    int *arguments = new int[length];
    for (unsigned int i = 0; i < length; i++) { arguments[i] = argTypes[i]; }
    arguments[length - 1] = 0;

    // Determine if the command exists or not.  If it does, it is the first time seeing it
    // Therefore we need to add the definition the mapping object
    if (m_serverFunctionMap.find(command) == m_serverFunctionMap.end()) {
        // Recreate command with new arguments and create server-function map entry
        command = rpc_info(name, arguments);
        m_serverFunctionMap[command] = new list<function_info*>();
    }

    // Get the list of servers associated with this command
    list<function_info*> *supportedServers = m_serverFunctionMap[command];

    // Create the server-function entry for the command
    rpc_info *sfnc_command = new rpc_info(name, arguments);
    function_info *server_func = new function_info(server_identifier, port, sfnc_command);

    // Iterate through the list of servers that currently support this command
    // If any of them match the current server, then kick them out
    for (list<function_info *>::iterator it = supportedServers->begin(); it != supportedServers->end(); it++) {
        function_info *stale_function_info = *it;

        if (*stale_function_info == *server_func) {
            // Remove stale entry
            supportedServers->remove(stale_function_info);
            delete stale_function_info;

            // overwrite the stale entry
            result = FUNCTION_OVERRIDDEN;

            break;
        }
    }

    // Add the new/updated server_func
    supportedServers->push_back(server_func);
    return result;
}

// Registers a server with the binder based on the socket address information.
void server_register(string server_identifier, unsigned short port, int serverfd) {
    server_info* server = new server_info(server_identifier, port);
    bool server_known = false;

    // Determine if we already know this server or not
    for (auto const &knownServer : m_priorityQueue) {
        if (*knownServer == *server) {
            server_known = true;
        }
    }

    // If we know the server, do not add
    if (server_known) {
        delete server;
        return;
    }

    // If we know server, then add
    m_priorityQueue.push_back(server);
    m_socketServerMap[serverfd] = server;    
}

// Removes a known server from the binder map based on the socket descriptor.
void server_remove(int serverfd) {
    // Check if any matching servers to file descriptor, if none exit
    if (m_socketServerMap.find(serverfd) == m_socketServerMap.end()) {
        return;
    }

    // Get the server matching the descriptor
    server_info *server = m_socketServerMap[serverfd];

    // Remove it from the priority queue
    m_priorityQueue.remove(server);

    // Lookup the server and then remove it from map (essentially remove_if_exists)
    map<int, server_info *>::iterator server_pos = m_socketServerMap.find(serverfd);
    if (server_pos != m_socketServerMap.end()) {
        m_socketServerMap.erase(server_pos);
    }

    // Iterate over map and erase from any of the supporting
    for (auto const &rpc_map : m_serverFunctionMap) {
        list<function_info*>* server_list = rpc_map.second;
        for (list<function_info*>::iterator it = server_list->begin(); it != server_list->end(); it++) {

            function_info* info = *it;
            if (*server == *info) {
                server_list->erase(it);
                break;
            }
        }
    }

    // Delete the server as it is on heap
    delete server;

    // Wipe any information about it from message information
    m_messageBlocks[serverfd] = 0;
    m_messageInfo[serverfd] = FAILURE;
}

// Handles an registration request by a server.
void handleRegisterRequest(Protocol& handler, BinaryStream& stream, int serverfd) {
    try {
        // Get name of the server, port and rpc name
        string server_identifier = stream.readString();
        unsigned short port = stream.readUInt16();
        string name = stream.readString();

        // Read the number of arguments
        unsigned int argTypesLength = stream.readUInt32();
        int argTypes[argTypesLength];
        stream.readInt32(argTypes, argTypesLength);

        // Register the server and add the function to the support list
        server_register(server_identifier, port, serverfd);
        ReasonCode result = function_add(name, argTypes, server_identifier, port);

        // send success
        handler.sendRegisterResponse(result);
    }
    catch (int e) {
        // Remove the server completely to ensure
        //no issues related to any of the commands
        server_remove(serverfd);

        // send register error
        handler.sendRegisterError(ReasonCode::ERROR);
    }
}

// Handles an incoming cache request for a server locations.
void handleLocationCacheRequest(Protocol& handler, BinaryStream& stream) {
    try {
        // Get the name of the function
        string name = stream.readString();

        // Get the arguments
        unsigned int argTypesLength = stream.readUInt32();
        int argTypes[argTypesLength];
        stream.readInt32(argTypes, argTypesLength);

        // Construct an rpc definition
        rpc_info rpc(name, argTypes);

        // Using the rpc definition, we lookup to see if we actually know any
        // servers that support this function
        if(m_serverFunctionMap.find(rpc) != m_serverFunctionMap.end()) {
            list<function_info*>* supportedServers = m_serverFunctionMap[rpc];

            // If the list is not empty, set the list to the user
            if (supportedServers->size() > 0) {
                handler.sendLocationCacheResponse(*supportedServers);
            }
            else {
                // if the list is empty, inform client that we have nothing available
                handler.sendLocationCacheError(FUNCTION_NOT_AVAILABLE);
            }
        }
        else {
            // we couldn't find any known supported servers, therefore nothing available
            handler.sendLocationCacheError(FUNCTION_NOT_AVAILABLE);
        }
    }
    catch (int e) {
        // An error happened, so throw a failure message
        handler.sendLocationCacheError(ERROR);
    }
}

// Handles an incoming request for a server location.
void handleLocationRequest(Protocol& handler, BinaryStream& stream) {
    try {
        // read function name
        string name = stream.readString();

        // Get the arguments
        unsigned int argTypesLength = stream.readUInt32();
        int argTypes[argTypesLength];
        stream.readInt32(argTypes, argTypesLength);

        // Construct a rpc definition
        rpc_info rpc(name, argTypes);

        // Get a server based on the priority queue (round robin)
        server_info *location = getPriorityServer(rpc);
        if (location != NULL) {
            // Send the server and port that we got
            handler.sendLocationResponse(location->server_identifier, location->port);
        }
        else {
            // We could not find an appropriate server for the function
            handler.sendLocationError(FUNCTION_NOT_AVAILABLE);
        }
    }
    catch (int e) {
        handler.sendLocationError(ERROR);
    }
}

// Handles a termination request to the binder.
void handleTerminateRequest() {
    // We are no longer running, instruct it to shutdown
    m_running = false;

    // We need to instruct each of the servers known to the binder to shutdown
    for (auto const& server : m_socketServerMap) {
        int serverfd = server.first;
        Protocol handler(serverfd);

        // instruct this server to shutdown
        handler.sendTerminate();
    }

    // Now that we have told all of the servers to shutdown, we need to wait around to make sure
    // we are the last machine to shutdown.  We will iterate until we can no longer ping any of the known servers

    // Store a list of servers we couldn't talk to so we are assuming they shutdown
    list<int> shutdown_list;
    while (m_socketServerMap.size() > 0) {
        for (auto const& socketServer : m_socketServerMap) {
            server_info *server = socketServer.second;
            int serverfd = socketServer.first;

            // Open a connection to this server, it should fail if the server is no longer running
            int socketfd = socket_create(server->server_identifier, server->port);
            if (socketfd < 0) {
                // Couldn't reach, add to 'likely' shutdown list
                shutdown_list.push_back(serverfd);
            }

            // We are done, close down descriptor
            close(socketfd);
        }

        //  Remove server from the list of known servers
        for (auto const sfd : shutdown_list) {
            server_remove(sfd);
        }

        // Clear the list of servers who we could not connect to
        shutdown_list.clear();

        // Wait for a period then try again
        usleep(1 * 1000 * 1000);
    }
}

void handleServerClose(int socketfd, fd_set *master_set) {
    // Cleanup server
    m_messageBlocks[socketfd] = 0;
    server_remove(socketfd);

    // Cleanup socket
    FD_CLR(socketfd, master_set);
    close(socketfd);
}

// Handles a recently received request from a currently open socket file descriptor
void handleRequest(int socketfd, fd_set *master_set) {
    // Establish the protocol controller for the requests
    Protocol handler(socketfd);
    unsigned int messageSize;

    // Check to see if any bytes have been read and are available for processing
    // If no bytes, then handle receiving, if bytes then handle request
    if (m_messageBlocks[socketfd] != 0) {
        // Bytes are available for processing
        unsigned int size = m_messageBlocks[socketfd];

        // Allocate a buffer for storing the received bytes
        char buffer[size];
        int status = handler.receiveMessage(size, buffer);

        // If completed with no errors, then we need to handle the specific message
        if (status != 0) {
            handleServerClose(socketfd, master_set);
            return;
        }

        m_messageBlocks[socketfd] = 0;
        MessageType msgType = m_messageInfo[socketfd];

        // Create the BinaryStream object for reading binary
        BinaryStream stream(buffer, size);

        // Handle message based on type
        switch(msgType) {
            case REGISTER:
                handleRegisterRequest(handler, stream, socketfd);
                break;
            case LOC_REQUEST:
                handleLocationRequest(handler, stream);
                break;
            case LOC_CACHE_REQUEST:
                handleLocationCacheRequest(handler, stream);
                break;
            case TERMINATE:
                // This will probably not be called as
                // we terminate the moment we get it, but it fills out
                // the switch statement
                handleTerminateRequest();
                break;
            default:
                break;
        }
    }
    else {
        //  Get message from socket, if error then close the socket
        if(handler.receiveMessageSize(messageSize) != 0 ) {
            handleServerClose(socketfd, master_set);
            return;
        }

        // Set bytes pending into map
        m_messageBlocks[socketfd] = messageSize;

        // Get message type from socket
        MessageType msgType;
        if(handler.receiveMessageType(msgType) != 0) {
            handleServerClose(socketfd, master_set);
            return;
        }

        // Update message info from socket
        m_messageInfo[socketfd] = msgType;

        // If type is terminate, we need to handle that immediately
        if (msgType == TERMINATE) {
            handleTerminateRequest();
        }
    }
}

// Formatted method for simply printing the binder information
void print_info(string hostname, int port) {
    std::cout << "BINDER_ADDRESS " << hostname << std::endl;
    std::cout << "BINDER_PORT " << port << std::endl;
}

// Prints the socket hostname and port
void print_settings(int socketfd) {
    string hostname = getHostname();
    int portno = getPort(socketfd);

    print_info(hostname, portno);
}

//--------------------------------------------------------------------------------------

// Entrypoint
int main(int argc, char *argv[]) {

    // Allocate a socket on any port, if failed exit
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        return SOCKET_OPEN_ERROR;
    }

    // ignoring an annoying signal called sigpipe, essentially if you are working with sockets
    // if you try to do some reading from a closed socket, you get a SIGPIPE signal.  In a multi-socket software, we don't care.
    signal(SIGPIPE, SIG_IGN);

    // Initialize listening on the newly opened socket
    socket_listen(socketfd);

    // Print the settings (hostname / port)
    print_settings(socketfd);

    // Establishes the file descriptor sets for monitoring incoming
    // user connections
    int max_fd = socketfd;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    FD_ZERO(&working_set);

    // Add opened socket to master set
    FD_SET(socketfd, &master_set);

    m_running = true;
    while (m_running) {
        // Copy master set onto the working set
        memcpy(&working_set, &master_set, sizeof(master_set));

        // Iterate through the working set for max file descriptor
        int selectResult = select(max_fd + 1, &working_set, NULL, NULL, NULL);

        // On failure exit
        if (selectResult < 0) {
            return SELECT_FAILURE;
        }

        // On timeout exit
        if (selectResult == 0) {
            return SELECT_TIMEOUT;
        }

        // Iterate through the working set testing each if in working set
        for (int i = 0; i < max_fd + 1; i++) {
            if (FD_ISSET(i, &working_set)) {
                // if socket is listening socket, we have a connection
                if (i == socketfd) {
                    // add the new connection to master_set
                    int new_connection = socket_accept(socketfd);
                    if(new_connection > max_fd) {
                        max_fd = new_connection;
                    }
                    FD_SET(new_connection, &master_set);
                }
                else {
                    // Existing connection has a request, handle it
                    int client_socketfd = i;
                    handleRequest(client_socketfd, &master_set);

                    if (!m_running) {
                        break;
                    }
                }
            }
        }
    }

    // Closes the socket
    close(socketfd);

    return 0;
}