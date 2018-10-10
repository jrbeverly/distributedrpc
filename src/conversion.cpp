#include "conversion.h"
#include "rpcinfo.h"
#include "helpers.h"

#include <cstdint>
#include <iostream>
#include <cstring>
#include <string>

using namespace std;

#define SIZEOF_64 8
#define SIZEOF_32 4
#define SIZEOF_16 2
#define SIZEOF_8 1

// Sticking to simply using memcpy and union hack.  These do not preserve well, but
// I got tired of dealing with bitshift code
//--------------------------------------------------------------------------------------

int16_t Convert::parseInt16(const char buffer[]) {
    int16_t value;
    memcpy(&value, buffer, sizeof(int16_t));
    return value;
}

int32_t Convert::parseInt32(const char buffer[]) {
    int32_t value;
    memcpy(&value, buffer, sizeof(int32_t));
    return value;
}

int64_t Convert::parseInt64(const char buffer[]) {
    int64_t value;
    memcpy(&value, buffer, sizeof(int64_t));
    return value;
}

uint16_t Convert::parseUInt16(const char buffer[]) {
    uint16_t value;
    memcpy(&value, buffer, sizeof(uint16_t));
    return value;
}

uint32_t Convert::parseUInt32(const char buffer[]) {
    uint32_t value;
    memcpy(&value, buffer, sizeof(uint32_t));
    return value;
}

uint64_t Convert::parseUInt64(const char buffer[]) {
    uint64_t value;
    memcpy(&value, buffer, sizeof(uint64_t));
    return value;
}

float Convert::parseFloat(const char buffer[]) {
    FloatMapping mapping;
    for (int i = 0; i < SIZEOF_32; i++) {
        mapping.bytes[i] = buffer[i];
    }
    return mapping.value;
    /*
    const unsigned char *b = (const unsigned char *)buf;
    uint32_t temp = 0;
    *i += 4;
    temp = ((b[0] << 24) |
            (b[1] << 16) |
            (b[2] <<  8) |
             b[3]);
    return *((float *) temp);
    */
}

double Convert::parseDouble(const char buffer[]) {
    DoubleMapping mapping;
    for (int i = 0; i < SIZEOF_64; i++) {
        mapping.bytes[i] = buffer[i];
    }
    return mapping.value;
}

//------------------------------------------------------------------------

void BitConverter::serializeInt16(int16_t value, char buffer[]) {
    memcpy(buffer, &value, sizeof(int16_t));
}

void BitConverter::serializeInt32(int32_t value, char buffer[]) {
    memcpy(buffer, &value, sizeof(int32_t));
}

void BitConverter::serializeInt64(int64_t value, char buffer[]) {
    memcpy(buffer, &value, sizeof(int64_t));
}

void BitConverter::serializeUInt16(uint16_t value, char buffer[]) {
    memcpy(buffer, &value, sizeof(uint16_t));
}

void BitConverter::serializeUInt32(uint32_t value, char buffer[]) {
    memcpy(buffer, &value, sizeof(uint32_t));
}

void BitConverter::serializeUInt64(uint64_t value, char buffer[]) {
    memcpy(buffer, &value, sizeof(uint64_t));
}

void BitConverter::serializeFloat(float value, char buffer[]) {
    FloatMapping mapping;
    mapping.value = value;
    for (int i = 0; i < SIZEOF_32; i++) {
        buffer[i] = mapping.bytes[i];
    }
}

void BitConverter::serializeDouble(double value, char buffer[]) {
    DoubleMapping mapping;
    mapping.value = value;
    for (int i = 0; i < SIZEOF_64; i++) {
        buffer[i] = mapping.bytes[i];
    }
}