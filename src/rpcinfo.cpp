#include "constants.h"
#include "rpcinfo.h"
#include "helpers.h"

#include <iostream>
#include <cstring>

using namespace std;

// Direct equiality comparison
bool operator ==(const function_info& service1, const function_info& service2) {
    if (service1.rpcdef->name != service2.rpcdef->name) {
        return false;
    }

    if (service1.server_identifier != service2.server_identifier || service1.port != service2.port) {
        return false;
    }

    // Get argument definitions
    int* argTypes1 = service1.rpcdef->argTypes;
    int* argTypes2 = service2.rpcdef->argTypes;

    // Get length of args
    int length1 = getArgTypesLength(argTypes1);
    int length2 = getArgTypesLength(argTypes2);
    if (length1 != length2) {
        return false;
    }

    // Iterate through the args
    for (int i = 0; i < length1 - 1; i++) {
        if (argTypes1[i] != argTypes2[i]) {
            return false;
        }
    }

    return true;
}

bool operator ==(const server_info& server, const function_info& service) {
    // Check socket addressing information
    return (server.server_identifier == service.server_identifier && server.port == service.port);
}

bool operator ==(const server_info &server1, const server_info &server2) {
    // Check socket addressing information
    return (server1.server_identifier == server2.server_identifier && server1.port == server2.port);
}


bool operator <(const rpc_info &rpc1, const rpc_info &rpc2) {
    // For the map<> object we need have defined the operator<
    // As such we are going to both sort the list, but our primary
    // goal being that rpc1 == rpc2 => !(rpc1 < rpc2) && !(rpc2 < rpc1) is false
    // when equiality (so we can do the 'find' function)

    // compare names
    if (strcmp(rpc1.name.c_str(), rpc2.name.c_str()) != 0) {
        return (rpc1.name < rpc2.name);
    }

    // Get argtypes
    int* argTypes1 = rpc1.argTypes;
    int* argTypes2 = rpc2.argTypes;

    // The number of arguments in the array
    int length1 = getArgTypesLength(argTypes1);
    int length2 = getArgTypesLength(argTypes2);

    // Order by number of args
    if (length1 != length2) {
        return length1 < length2;
    }

    // Same length, does not matter
    int length = length1;
    for (int i = 0; i < length - 1; i++) {
        if (argTypes1[i] == argTypes2[i]) {
            continue;
        }
        
        int size1 = getArgTypeArrayLength(argTypes1[i]);
        int size2 = getArgTypeArrayLength(argTypes2[i]);
        if (size1 != size2) {

            // We do not care for length of array (as it can change), only if it is
            // SCALAR vs ARRAY.  (again array is dynamic length, so we only care if scalar or not)
            if ((size1 == 0 && size2 != 0) || (size1 != 0 && size2 == 0)) {
                return argTypes1[i] < argTypes2[i];
            }
        }
    }

    // Needs to be false as map has this odd functionality where it wants the operator<.  If a < b and b < a both are false, then equiality.
    // Better solution would be a IComparable (e.g. -1, 0, 1)
    return false;
}