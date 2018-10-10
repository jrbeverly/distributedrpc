#pragma once

// TODO: Remove constants.h to other

//--------------------------------------------------------------------------------------
// Specifies constants defining message types.
enum MessageType {
    FAILURE = 0,
    TERMINATE = 10,

    REGISTER = 11,
    REGISTER_SUCCESS = 21,
    REGISTER_FAILURE = 41,

    EXECUTE = 12,
    EXECUTE_SUCCESS = 22,
    EXECUTE_FAILURE = 42,

    LOC_REQUEST = 13,
    LOC_SUCCESS = 23,
    LOC_FAILURE = 43,

    LOC_CACHE_REQUEST = 14,
    LOC_CACHE_SUCCESS = 24,
    LOC_CACHE_FAILURE = 44
};

//--------------------------------------------------------------------------------------
// Contains the values of status codes defined for RPC.
enum ReasonCode {
    SUCCESS = 0,
    ERROR = -1,

    RECEIVED_TERMINATED=-500,

    FUNCTION_OVERRIDDEN = 201,
    FUNCTION_NOT_AVAILABLE = -404,
    FUNCTION_EXECUTION_ERROR = -405,

    SOCKET_OPEN_ERROR = -300,
    SOCKET_UNKNOWN_HOST = -301,
    SOCKET_BIND_ERROR = -302,
    SOCKET_ACCEPT_ERROR = -303,
    SOCKET_SEND_ERROR = -307,
    SOCKET_RECEIVE_ERROR = -308,
    SOCKET_CONNECTION_ERROR = -310,

    EXECUTE_UNKNOWN_SKELETON = -202,

    INIT_BINDER_ADDRESS_NOT_SET = -501,
    INIT_BINDER_PORT_NOT_SET = -502,
    INIT_BINDER_SOCKET_ERROR = -503,
    INIT_SOCKET_ERROR = -504,

    SELECT_FAILURE = -550,
    SELECT_TIMEOUT = -558,

    RECEIVE_INVALID_COMMAND_NAME = -700,
    RECEIVE_INVALID_MESSAGE_TYPE = -701,
    RECEIVE_INVALID_MESSAGE = -702
};

// These codes are inspired by HTTP.  Grouping with 1xx, 2xx, 3xx based on type of message
// with 404 remaining as the 'not found - not available'