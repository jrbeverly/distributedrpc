/*
 * rpcServer.cpp
 *
 * This file implements the server-side code associated with rpc.h
 */
#include "rpc.h"
#include "constants.h"
#include "helpers.h"
#include "bstream.h"
#include "conversion.h"
#include "protocol.h"

#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <limits.h>
#include <math.h>
#include <cstring>
#include <map>
#include <signal.h>

using namespace std;

// Store a list of threads that are handling responses from the clients
static pthread_mutex_t* m_listLock;
static map<pthread_t, int> m_threadPool;

// List of functions that are registered with the server
// We exploit the fact that we make an operator< for the rpc_info
// thus allowing us to use an rpc_info as a key in a bunch of differing structs
static map<rpc_info, skeleton> m_registeredRpc;

//global variables
int serverPort = 0;
int serverfd = -1;
std::string servname = getHostname();
int binderfd = -1;

//attribute for pthreads
//make joinable
pthread_attr_t attr;

//int is socket fd,thread is pthread
static map<int,pthread_t> th;

//node structure for linked list
struct node {
    char *name;
    int *argTypes;
    skeleton f;
    node *next;
};

//function list to have head and tail of nodes
struct funcList {
    node* head;
    node* tail;
    int numFun = 0;
};
//structure to store information about message to
//store as void * for threads
struct msgInfo {
    int msgSize;
    int idthr;
};

//as SOMAXCONN clients and some fds not used at all
//call msgInfo based on connected socketfd for thread
//+ 10 just to be safe
struct msgInfo msgLst[SOMAXCONN + 10];

//has list of all the functions
struct funcList *lst = new funcList;

//creates the server
int rpcInit() {
    serverfd = socket(AF_INET, SOCK_STREAM, 0);

    signal(SIGPIPE, SIG_IGN);
    if (serverfd < 0) {
        //return error as whatever serverfd's error is
        return serverfd;
    }

    // set server socket to the listening state
    socket_listen(serverfd);
    serverPort = getPort(serverfd);

    // Get binder address information
    string binderAddress = getBinderAddress();
    int binderPort = getBinderPort();

    // Connect to binder
    binderfd = socket_create(binderAddress, binderPort);

    //checking if error occured and returning error
    if (binderfd < 0) {
        return INIT_BINDER_SOCKET_ERROR;
    }

    // For the purposes of the server, we need to be able to handle requests from multiple clients
    // therefore, create a listlock and initialize it
    m_listLock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m_listLock, NULL);

    return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton fnc_skeleton) {
    Protocol handler(binderfd);
    int status = 0;

    //sending data for register
    status = handler.sendRegister(servname, serverPort, name, argTypes);
    if (status < 0) {
        return status;
    }

    // --------
    // Prepare to receive the register response
    // By specification: LENGTH, TYPE, MESSAGE (contents)

    MessageType type;
    unsigned int msgSize;

    // Getting type and size
    status = handler.receiveMessageSize(msgSize);
    if (status < 0) {
        return status;
    }

    status = handler.receiveMessageType(type);
    if (status < 0) {
        return status;
    }

    // Allocate buffer for message
    char msg[msgSize];
    status = handler.receiveMessage(msgSize, msg);
    if (status < 0) {
        return status;
    }

    // Wrapper over the stream
    BinaryStream stream(msg, msgSize);

    if (type == REGISTER_SUCCESS) {
       int reasonCode = stream.readInt32();

       struct rpc_info rpc(string(name), argTypes);
       m_registeredRpc[rpc] = fnc_skeleton;

       return reasonCode;
    }
    else if (type == REGISTER_FAILURE) {
        int reasonCode = stream.readInt32();
        return reasonCode;
    }
    else {
        return RECEIVE_INVALID_MESSAGE_TYPE;
    }
    
    return 0;
}

//deleting function list on termination
//as well as closing threads
void rem_lst() {
    //deleting lst if no nodes
    if (lst->numFun == 0) {
        delete lst;
    }
    //deleting lst if >=1 node
    else {
        struct node *remNode = lst->head;
        for (int i =0; i < lst->numFun; i++) {
            struct node *nextNode = remNode->next;
            delete remNode;
            remNode = nextNode;
        }
        delete lst;
    }

    //deleting threads
    //loop through threads and call thread_join
    for (auto const thItem : th) {

        pthread_t cur = thItem.second;
        pthread_join(cur, NULL);
    }
}

int execute_request(int msgSize, Protocol handle) {
    //creating message variable and reading from it
    char msg[msgSize];
    int status = handle.receiveMessage(msgSize, msg);

    //checking if error in receiving message
    if (status != 0) {
        return status;
    }

    //reading data
    BinaryStream stream(msg, msgSize);
    string name = stream.readString();
    unsigned int argLen = stream.readUInt32();
    int argTypes[argLen];
    stream.readInt32(argTypes, argLen);
    void *args[argLen];

    //getting argument with loop based on argtype
    for (int i = 0; i < argLen-1; i++) {
        int argType = argTypes[i];
        int ctype = getArgType(argType);
        int length = getArgTypeArrayLength(argType);
        if (length == 0) {
            length = 1;
        }

        switch(ctype) {
            case ARG_CHAR: {
                    char* charVal = new char[length];
                    stream.readChar(charVal, length);
                    args[i] = (void *) charVal;
                    break;
                }
            case ARG_SHORT: {
                    short* shortVal = new short[length];
                    stream.readInt16( shortVal, length);
                    args[i] = (void *) shortVal;
                    break;
                }
            case ARG_INT: {
                    int* intVal = new int[length];
                    stream.readInt32( intVal, length);
                    args[i] = (void *) intVal;
                    break;
                }
            case ARG_LONG: {
                    long* longVal = new long[length];
                    stream.readInt64( longVal, length);
                    args[i] = (void *) longVal;
                    break;
                }
            case ARG_DOUBLE: {
                    double* dbVal = new double[length];
                    stream.readDouble( dbVal, length);
                    args[i] = (void *) dbVal;
                    break;
                }
            case ARG_FLOAT:      {
                    float* fltVal = new float[length];
                    stream.readFloat( fltVal, length);
                    args[i] = (void *) fltVal;
                    break;
                }
            default:
                break;
        }
    }

    //find function in list
    int numFunc = lst->numFun;
    struct node *curNode = lst->head;
    //substring as strings normally include '\0' character causing the compare to fail
    name = name.substr(0, name.length()-1);

    //seeing if function exists in list
    while (curNode != NULL) {
        string curNodeName = curNode->name;

        if ((!name.compare(curNodeName)) &&
                (same_int_arr(argTypes,curNode->argTypes) ==0 )) {
            break;
        }
        curNode = curNode->next;
    }
    if (curNode == NULL) {
        return FUNCTION_NOT_AVAILABLE;
    }


    //calling skeleton
    skeleton curFunc = curNode->f;
    int exeResult = (*curFunc) (argTypes, args);

    //returning error if function doesn't execute properly
    if (exeResult != 0) {
        handle.sendExecuteError(FUNCTION_EXECUTION_ERROR);
    }
    //sending scucess message to user
    else {
        handle.sendExecuteResponse(curNode->name, argTypes, args);
    }

    return exeResult;
}

void *th_ex_req(void *buf) {
    struct msgInfo *mInfo;
    int thID;
    int msgSize;

    //getting variables
    mInfo = (struct msgInfo *) buf;
    msgSize = mInfo->msgSize;
    thID = mInfo->idthr;
    Protocol proto(thID);

    //seeing if execute_request is called properly
    int result = execute_request(msgSize, proto);

    if (result != 0) {
        ReasonCode rsCode = static_cast<ReasonCode>(result);
        proto.sendExecuteError(rsCode);
    }

    //removing thID from map
    th.erase(thID);
    //ending thread as it has done what it needs to
    pthread_exit(NULL);
}

// Threading 
void* thread_exec(void* arguments) {
    // Get arguments from the passed object
    void** array = (void**)arguments;

    int* socketfd = (int*)array[0];
    unsigned int* sizeArg = (unsigned int*)array[1];
    unsigned int msgSize = *sizeArg;
    char* buffer = (char*)array[2];

    Protocol handler(*socketfd);

    //reading data
    BinaryStream stream(buffer, msgSize);
    string name = stream.readString();
    unsigned int argLen = stream.readUInt32();
    int argTypes[argLen];
    stream.readInt32(argTypes, argLen);
    void *args[argLen];

    //getting argument with loop based on argtype
    for (int i = 0; i < argLen-1; i++) {
        int argType = argTypes[i];
        int ctype = getArgType(argType);
        int length = getArgTypeArrayLength(argType);
        if (length == 0) {
            length = 1;
        }

        switch(ctype) {
            case ARG_CHAR: {
                    char* charVal = new char[length];
                    stream.readChar(charVal, length);
                    args[i] = (void *) charVal;
                    break;
                }
            case ARG_SHORT: {
                    short* shortVal = new short[length];
                    stream.readInt16( shortVal, length);
                    args[i] = (void *) shortVal;
                    break;
                }
            case ARG_INT: {
                    int* intVal = new int[length];
                    stream.readInt32( intVal, length);
                    args[i] = (void *) intVal;
                    break;
                }
            case ARG_LONG: {
                    long* longVal = new long[length];
                    stream.readInt64( longVal, length);
                    args[i] = (void *) longVal;
                    break;
                }
            case ARG_DOUBLE: {
                    double* dbVal = new double[length];
                    stream.readDouble( dbVal, length);
                    args[i] = (void *) dbVal;
                    break;
                }
            case ARG_FLOAT:      {
                    float* fltVal = new float[length];
                    stream.readFloat( fltVal, length);
                    args[i] = (void *) fltVal;
                    break;
                }
            default:
                break;
        }
    }
    
    // Get the rpc information
    struct rpc_info rpc(name, argTypes);
    map<rpc_info, skeleton>::iterator skel_pos = m_registeredRpc.find(rpc);

    ReasonCode reasonCode = SUCCESS;

    // For testing early termination
    //sleep(2);

    // Unknown rpc
    if(skel_pos == m_registeredRpc.end()) {        
        reasonCode = EXECUTE_UNKNOWN_SKELETON;
    }
    else {
        skeleton func_skeleton = skel_pos->second;

        // call the skeleton
        int result = func_skeleton(argTypes, args);
        if (result == 0) {           
            result = handler.sendExecuteResponse(name, argTypes, args);
        }
        else {
            // set failure response
            reasonCode = static_cast<ReasonCode>(result);
        }
    }

    // if it is not success, then send execute error
    if(reasonCode != SUCCESS) {
        int result = handler.sendExecuteError(reasonCode);
    }

    // TODO: Delete all the arrays from above

    delete socketfd;
    delete sizeArg;
    delete [] buffer;

    // Remove from threadpool
    pthread_mutex_lock(m_listLock);
    m_threadPool.erase(pthread_self());
    pthread_mutex_unlock(m_listLock);    

    return NULL;
}

int handleRequest(int j, fd_set* master_set) {
    Protocol proto(j);
    bool success = false;
    int status = 0;

    unsigned int msgSize;
    MessageType type;

    //types to receive data for requests
    status = proto.receiveMessageSize(msgSize);
    if (status != 0) {
        FD_CLR(j, master_set);
        close(j);
        return 0;
    }

    status = proto.receiveMessageType(type);
    if (status != 0) {
        FD_CLR(j, master_set);
        close(j);
        return 0;
    }

    if(binderfd == j && type == TERMINATE) {
        // Terminate only if it is the binder
        return -1;
    }

    if (type != EXECUTE) {
        FD_CLR(j, master_set);
        close(j);
        return 0;
    }

    char buffer[msgSize];
    status = proto.receiveMessage(msgSize, buffer);
    if (status != 0) {
        FD_CLR(j, master_set);
        close(j);
        return 0;
    }

    // Prepare arguments for the thread
    pthread_t rthread;
    void ** arguments = new void*[3];

    // Create copies of arguments on heap
    int * copy_socketfd = new int(j);
    unsigned int * copy_msgSize = new unsigned int(msgSize);
    char * copy_buffer = new char[msgSize];
    memcpy (copy_buffer, buffer, msgSize);
    
    // add to arguments
    arguments[0] = (void *)copy_socketfd;
    arguments[1] = (void *)copy_msgSize;
    arguments[2] = (void *)copy_buffer;

    // Start thread                    
    pthread_create(&rthread, NULL, &thread_exec, (void *)arguments);
    m_threadPool[rthread] = j;

    return 0;
}

int rpcExecute() {
    // We have no functions
    if (m_registeredRpc.size() == 0) { 
        return FAILURE; 
    }

    if (serverfd < 0) {
        return SOCKET_RECEIVE_ERROR;
    }

    // Establishes the file descriptor (is binder as binder is registered second)
    int max = binderfd;
    fd_set master, read_fds;
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // Add server and binder to set
    FD_SET(serverfd, &master);
    FD_SET(binderfd, &master);

    bool running = true;
    int requestCode = 0;
    while (running) {
        //making copy of master in read_fds
        memcpy(&read_fds, &master, sizeof(master));

        int selectResult = select(max + 1, &read_fds, NULL, NULL, NULL);

        // Check to see if binder is still around
        {
             int test_binderPort = getBinderPort();
             std::string test_binderAddr = getBinderAddress();
 
             int test_binderfd = socket_create(test_binderAddr, test_binderPort);
             if (test_binderfd < 0) {
                 running = false;                 
             }
             
             close(test_binderfd);
        }

        // On failure exit
        if (selectResult < 0) {
            return SELECT_FAILURE;
        }

        // On timeout exit
        if (selectResult == 0) {
            return SELECT_TIMEOUT;
        }
        
        // Iterate through the working set testing each if in working set        
        for (int j = 0; j < max + 1; j++) {
            if (FD_ISSET(j, &read_fds)) {                
                if (j == serverfd) {
                    int newfd = socket_accept(serverfd);
                    if (newfd > max) {
                        max = newfd;
                    }

                    FD_SET(newfd, &master);
                }
                else {
                    requestCode = handleRequest(j, &master);
                    if (requestCode < 0) {
                        running = false;
                    }

                    if (!running) { 
                        break;
                    }
                }
            }
        }
    }

    pthread_mutex_lock(m_listLock);
    for(auto const& pair : m_threadPool) {
        // kill thread
        pthread_cancel(pair.first);

        // talk to client, tell it we are done
        Protocol handler(pair.second);
        handler.sendExecuteError(ReasonCode::RECEIVED_TERMINATED);
        
        // close the pair
        close(pair.second);
    }
    pthread_mutex_unlock(m_listLock);

    close(serverfd);
    close(binderfd);

    //exiting
    return requestCode;
}
