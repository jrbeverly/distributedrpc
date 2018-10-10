
struct server_info {
    std::string server_identifier;
    unsigned short port;
};

//######################

struct command_register {
    std::string server_identifier;
    unsigned short port;
    std::string name;
    std::vector<int> argTypes;
};

struct command_register_response {
    ReasonCode code;
};

struct command_register_error {
    ReasonCode code;
};

//######################

struct command_location_cache {
    std::string name;
    std::vector<int> argTypes;
};

struct command_location_cache_response {
    std::vector<function_info*> &services;
};

struct command_location_cache_error {
    ReasonCode code;
};

//######################

struct command_location {
    std::string name;
    std::vector<int> argTypes;
};

struct command_location_response {
    std::string server_identifier;
    unsigned short port;
};

struct command_location_error {
    ReasonCode code;
};

//######################

struct command_execute {
    std::string name;
    std::vector<int> argTypes;
    std::vector<void*> argTypes;
};

struct command_execute_response {
    std::string name;
    std::vector<int> argTypes;
    std::vector<void*> argTypes;
};

struct command_execute_error {
    ReasonCode code;
};
