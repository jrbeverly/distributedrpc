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
    
    void writeChar(char value);
    void writeChar(const char value[], int length);

    char readChar();
    void readChar(char array[], unsigned int length);

  private:
    std::vector<char> m_bytes;
    int m_position;
};