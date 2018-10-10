class BinaryWriter {
  public:

    BinaryStream(MemoryStream stream);

    void writeChar(char value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    
    void writeString(std::string value);
    void writeCString(const char* value);

    void writeChar(const char value[], int length);
    void writeInt16(short value[], int length);
    void writeInt32(int value[], int length);
    void writeInt64(long value[], int length);
    void writeFloat(float value[], int length);
    void writeDouble(double value[], int length);

  private:
    MemoryStream m_stream;
};