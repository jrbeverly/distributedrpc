#pragma once

// TODO: BinaryReader, BinaryWriter, MemoryStream

/*
bstream.cpp

This class is derived from the concept of a MemoryStream / BinaryReader and BinaryWriter from the CSharp (C#) language.
These classes help facilitate the process of working with byte buffers, and help abstract away any issues with byte-size / endianness
that would come with sending data over a network.

As the interface is straightforward and modelled after the CSharp equivalent, it makes the process of writing/reading much easier.

See the interfaces that this class is derived from as well as the source for the documentation of each of the methods.
  * https://msdn.microsoft.com/en-us/library/system.io.memorystream(v=vs.110).aspx
  * https://msdn.microsoft.com/en-us/library/system.io.binaryreader(v=vs.110).aspx
  * https://msdn.microsoft.com/en-us/library/system.io.binarywriter(v=vs.110).aspx
*/

#include <string>
#include <cstdint>
#include <vector>

// Creates a stream whose backing store is memory.
class BinaryStream {
  public:

    // Initializes a new instance of the BinaryStream class with an expandable capacity initialized to zero.
    BinaryStream();

    // Initializes a new instance of the BinaryStream class with an expandable capacity initialized as specified.
    BinaryStream(int size);

    // Initializes a new instance of the BinaryStream class based on the specified region (length) of a byte array.
    BinaryStream(char* buffer, int size);

    // returns buffer
    char* str();
    char* buffer();
    void seek(int index);
    void reset();

    // Gets the length of the stream in bytes.
    int size();

    //--------------------------------------------------------------------------------------
    // Writing

    // Writes a character to the current stream and advances the current position of the stream.
    void writeChar(char value);

    // Writes a two-byte signed integer to the current stream and advances the stream position by two bytes.
    void writeInt16(int16_t value);

    // Writes a four-byte signed integer to the current stream and advances the stream position by four bytes.
    void writeInt32(int32_t value);

    // Writes an eight-byte signed integer to the current stream and advances the stream position by eight bytes.
    void writeInt64(int64_t value);

    // Writes a two-byte unsigned integer to the current stream and advances the stream position by two bytes.
    void writeUInt16(uint16_t value);

    // Writes a four-byte unsigned integer to the current stream and advances the stream position by four bytes.
    void writeUInt32(uint32_t value);

    // Writes an eight-byte unsigned integer to the current stream and advances the stream position by eight bytes.
    void writeUInt64(uint64_t value);

    // Writes a four-byte floating-point value to the current stream and advances the stream position by four bytes.
    void writeFloat(float value);

    // Writes an eight-byte floating-point value to the current stream and advances the stream position by eight bytes.
    void writeDouble(double value);

    // Writes a length-prefixed string to this stream, and advances the current position of the stream.
    void writeString(std::string value);

    // Writes a null terminated string to this stream, and advances the current position of the stream.
    void writeCString(const char* value);

    //--------------------------------------------------------------------------------------

    // Writes a character array of the specified length to the current stream and advances the current position of the stream.
    void writeChar(const char value[], int length);

    // Writes a 2-byte signed integer array of the specified length to the current stream and advances the current position of the stream.
    void writeInt16(short value[], int length);

    // Writes a 4-byte signed integer array of the specified length to the current stream and advances the current position of the stream.
    void writeInt32(int value[], int length);

    // Writes a 8-byte signed integer array of the specified length to the current stream and advances the current position of the stream.
    void writeInt64(long value[], int length);

    // Writes a 4-byte floating-point array of the specified length to the current stream and advances the current position of the stream.
    void writeFloat(float value[], int length) ;

    // Writes a 8-byte floating-point array of the specified length to the current stream and advances the current position of the stream.
    void writeDouble(double value[], int length);


    //--------------------------------------------------------------------------------------
    // Reading

    // Reads the next character from the current stream and advances the current position of the stream.
    char readChar();

    // Reads a 2-byte signed integer from the current stream and advances the current position of the stream by two bytes.
    int16_t readInt16();

    // Reads a 4-byte signed integer from the current stream and advances the current position of the stream by four bytes.
    int32_t readInt32();

    // Reads an 8-byte signed integer from the current stream and advances the current position of the stream by eight bytes.
    int64_t readInt64();

    // Reads a 2-byte unsigned integer from the current stream and advances the position of the stream by two bytes.
    uint16_t readUInt16();

    // Reads a 4-byte unsigned integer from the current stream and advances the position of the stream by four bytes.
    uint32_t readUInt32();

    // Reads an 8-byte unsigned integer from the current stream and advances the position of the stream by eight bytes.
    uint64_t readUInt64();

    // Reads a 4-byte floating point value from the current stream and advances the current position of the stream by four bytes.
    float readFloat();

    // Reads an 8-byte floating point value from the current stream and advances the current position of the stream by eight bytes.
    double readDouble();

    // Reads a string from the current stream. The string is prefixed with the length, encoded as an four byte integer.
    std::string readString();

    // Reads a string from the current stream. The string is null terminated.
    std::vector<char> readCString();

    //--------------------------------------------------------------------------------------

    // Reads a character array of the specified length to the current stream and advances the current position of the stream.
    void readChar(char array[], unsigned int length);

    // Reads a 2-byte signed integer array of the specified length from the current stream and advances the current position of the stream.
    void readInt16(short array[], unsigned int length);

    // Reads a 4-byte signed integer array of the specified length from the current stream and advances the current position of the stream.
    void readInt32(int array[], unsigned int length);

    // Reads a 8-byte signed integer array of the specified length from the current stream and advances the current position of the stream.
    void readInt64(long array[], unsigned int length);

    // Reads a 4-byte floating-point array of the specified length from the current stream and advances the current position of the stream.
    void readFloat(float array[], unsigned int length);

    // Reads a 8-byte floating-point array of the specified length from the current stream and advances the current position of the stream.
    void readDouble(double array[], unsigned int length);

  private:
    std::vector<char> m_bytes;
    int m_position;
};