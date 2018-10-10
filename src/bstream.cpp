#include "bstream.h"
#include "conversion.h"

#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>

using namespace std;

//--------------------------------------------------------------------------------------

#define SIZEOF_64 8
#define SIZEOF_32 4
#define SIZEOF_16 2
#define SIZEOF_8 1

//--------------------------------------------------------------------------------------

BinaryStream::BinaryStream() {
    m_position = 0;
}

BinaryStream::BinaryStream(int size) {
    m_bytes.resize(size);
    m_position = 0;
}

BinaryStream::BinaryStream(char* buffer, int size) {
    for (int i = 0; i < size; i++) {
        m_bytes.push_back(buffer[i]);
    }
    m_position = 0;
}

//--------------------------------------------------------------------------------------

char* BinaryStream::str() {
    return &m_bytes.front();
}

char* BinaryStream::buffer() {
    return &m_bytes.front();
}

void BinaryStream::seek(int index) {
    m_position = index;
}

void BinaryStream::reset() {
    m_position = 0;
}

int BinaryStream::size() {
    return m_bytes.size();
}

//--------------------------------------------------------------------------------------

void BinaryStream::writeString(string value) {
    int length = value.length() + 1;
    writeInt32(length);

    for (char& character : value) {
        m_bytes.push_back(character);
    }
    m_bytes.push_back('\0');
}

void BinaryStream::writeCString(const char* value) {
    while (*value != '\0') {
        m_bytes.push_back(*value);
        value++;
    }
    m_bytes.push_back('\0');
}

std::string BinaryStream::readString() {
    int length = readInt32();
    string value;
    for (int i = 0; i < length; i++) {
        value += readChar();
    }
    return value;
}

std::vector<char> BinaryStream::readCString() {
    vector<char> chars;
    char value;
    do {
        value = readChar();
        chars.push_back(value);
    }
    while (value != '\0');

    return chars;
}

//--------------------------------------------------------------------------------------

void BinaryStream::writeChar(char value) {
    m_bytes.push_back(value);
}

void BinaryStream::writeInt16(int16_t value) {
    char bytes[SIZEOF_16];
    BitConverter::serializeInt16(value, bytes);

    for (int i = 0; i < SIZEOF_16; i++) {
        m_bytes.push_back(bytes[i]);
    }
}

void BinaryStream::writeInt32(int32_t value) {
    char bytes[SIZEOF_32];
    BitConverter::serializeInt32(value, bytes);

    for (int i = 0; i < SIZEOF_32; i++) {
        m_bytes.push_back(bytes[i]);
    }
}

void BinaryStream::writeInt64(int64_t value) {
    char bytes[SIZEOF_64];
    BitConverter::serializeInt64(value, bytes);

    for (int i = 0; i < SIZEOF_64; i++) {
        m_bytes.push_back(bytes[i]);
    }
}

void BinaryStream::writeUInt16(uint16_t value) {
    char bytes[SIZEOF_16];
    BitConverter::serializeUInt16(value, bytes);

    for (int i = 0; i < SIZEOF_16; i++) {
        m_bytes.push_back(bytes[i]);
    }
}

void BinaryStream::writeUInt32(uint32_t value) {
    char bytes[SIZEOF_32];
    BitConverter::serializeUInt32(value, bytes);

    for (int i = 0; i < SIZEOF_32; i++) {
        m_bytes.push_back(bytes[i]);
    }
}

void BinaryStream::writeUInt64(uint64_t value) {
    char bytes[SIZEOF_64];
    BitConverter::serializeUInt64(value, bytes);

    for (int i = 0; i < SIZEOF_64; i++) {
        m_bytes.push_back(bytes[i]);
    }
}

//--------------------------------------------------------------------------------------

void BinaryStream::writeFloat(float value) {
    // Converting to a string is probably not the best solution, but
    // I do not like using a union solution (undefined behaviour), and the
    // bit-shifting did not work as intended.  So here we are, we are slowing moving towards JSON.
    string as_string = std::to_string(value);
    writeCString(as_string.c_str());
    /*char bytes[SIZEOF_32];
    BitConverter::serializeFloat(value, bytes);

    for (int i = 0; i < SIZEOF_32; i++) {
        m_bytes.push_back(bytes[i]);
    }*/
}

void BinaryStream::writeDouble(double value) {
    // Converting to a string is probably not the best solution, but
    // I do not like using a union solution (undefined behaviour), and the
    // bit-shifting did not work as intended.  So here we are.

    string as_string = std::to_string(value);
    writeCString(as_string.c_str());
    /*char bytes[SIZEOF_64];
    BitConverter::serializeDouble(value, bytes);

    for (int i = 0; i < SIZEOF_64; i++) {
        m_bytes.push_back(bytes[i]);
    }*/
}

//--------------------------------------------------------------------------------------

char BinaryStream::readChar() {
    char value = m_bytes.at(m_position);
    m_position++;
    return value;
}

int16_t BinaryStream::readInt16() {
    char bytes[SIZEOF_16];
    for (int i = 0; i < SIZEOF_16; i++) {
        bytes[i] = m_bytes.at(m_position);
        m_position++;
    }

    return Convert::parseInt16(bytes);
}

int32_t BinaryStream::readInt32() {
    char bytes[SIZEOF_32];
    for (int i = 0; i < SIZEOF_32; i++) {
        bytes[i] = m_bytes.at(m_position);
        m_position++;
    }

    return Convert::parseInt32(bytes);
}

int64_t BinaryStream::readInt64() {
    char bytes[SIZEOF_64];
    for (int i = 0; i < SIZEOF_64; i++) {
        bytes[i] = m_bytes.at(m_position);
        m_position++;
    }

    return Convert::parseInt64(bytes);
}

uint16_t BinaryStream::readUInt16() {
    char bytes[SIZEOF_16];
    for (int i = 0; i < SIZEOF_16; i++) {
        bytes[i] = m_bytes.at(m_position);
        m_position++;
    }

    return Convert::parseUInt16(bytes);
}

uint32_t BinaryStream::readUInt32() {
    char bytes[SIZEOF_32];
    for (int i = 0; i < SIZEOF_32; i++) {
        bytes[i] = m_bytes.at(m_position);
        m_position++;
    }

    return Convert::parseUInt32(bytes);
}

uint64_t BinaryStream::readUInt64() {
    char bytes[SIZEOF_64];
    for (int i = 0; i < SIZEOF_64; i++) {
        bytes[i] = m_bytes.at(m_position);
        m_position++;
    }

    return Convert::parseUInt64(bytes);
}

//--------------------------------------------------------------------------------------

float BinaryStream::readFloat() {
    vector<char> cstr = readCString();
    std::string as_string(cstr.begin(), cstr.end());
    std::string::size_type sz;

    return std::stof (as_string, &sz);
}

double BinaryStream::readDouble() {
    vector<char> cstr = readCString();
    std::string as_string(cstr.begin(), cstr.end());
    std::string::size_type sz;

    return std::stod (as_string, &sz);
}


//--------------------------------------------------------------------------------------

void BinaryStream::writeChar(const char value[], int length) {
    for (int i = 0; i < length; i++) {
        writeChar(value[i]);
    }
}

void BinaryStream::writeInt16(short value[], int length) {
    for (int i = 0; i < length; i++) {
        writeInt16(value[i]);
    }
}

void BinaryStream::writeInt32(int value[], int length) {
    for (int i = 0; i < length; i++) {
        writeInt32(value[i]);
    }
}

void BinaryStream::writeInt64(long value[], int length) {
    for (int i = 0; i < length; i++) {
        writeInt64(value[i]);
    }
}

void BinaryStream::writeFloat(float value[], int length) {
    for (int i = 0; i < length; i++) {
        writeFloat(value[i]);
    }
}

void BinaryStream::writeDouble(double value[], int length) {
    for (int i = 0; i < length; i++) {
        writeDouble(value[i]);
    }
}

//--------------------------------------------------------------------------------------

void BinaryStream::readChar(char array[], unsigned int length) {
    for(int i = 0; i < length; i++) {
        array[i] = readChar();
    }
}

void BinaryStream::readInt16(short array[], unsigned int length) {
    for(int i = 0; i < length; i++) {
        array[i] = readInt16();
    }
}

void BinaryStream::readInt32(int array[], unsigned int length) {
    for(int i = 0; i < length; i++) {
        array[i] = readInt32();
    }
}

void BinaryStream::readInt64(long array[], unsigned int length) {
    for(int i = 0; i < length; i++) {
        array[i] = readInt64();
    }
}

void BinaryStream::readFloat(float array[], unsigned int length) {
    for(int i = 0; i < length; i++) {
        array[i] = readFloat();
    }
}

void BinaryStream::readDouble(double array[], unsigned int length) {
    for(int i = 0; i < length; i++) {
        array[i] = readDouble();
    }
}
