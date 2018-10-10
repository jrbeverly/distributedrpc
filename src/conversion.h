#pragma once

// TODO: Preserve

/*
As an aside:

This code went through multiple iterations, finally settling on converting between buffers based on memcpy and string conversion.

All because the C++ Standard states that the size of bytes depends on which architecture you're working on. Any other assumption is false. This is dumb.
*/

#include "rpcinfo.h"
#include "constants.h"

#include <cstdint>

using namespace std;

union FloatMapping {
    float value;
    char bytes[4];
};

union DoubleMapping {
    float value;
    char bytes[4];
};

// Converts an array of bytes to base data types.
namespace Convert {

    int16_t parseInt16(const char buffer[]);
    int32_t parseInt32(const char buffer[]);
    int64_t parseInt64(const char buffer[]);

    uint16_t parseUInt16(const char buffer[]);
    uint32_t parseUInt32(const char buffer[]);
    uint64_t parseUInt64(const char buffer[]);

    float parseFloat(const char buffer[]);
    double parseDouble(const char buffer[]);

}

// Converts base data types to an array of bytes.
namespace BitConverter {

    void serializeInt16(int16_t value, char buffer[]);
    void serializeInt32(int32_t value, char buffer[]);
    void serializeInt64(int64_t value, char buffer[]);

    void serializeUInt16(uint16_t value, char buffer[]);
    void serializeUInt32(uint32_t value, char buffer[]);
    void serializeUInt64(uint64_t value, char buffer[]);

    void serializeFloat(float value, char buffer[]);
    void serializeDouble(double value, char buffer[]);

}