#include "Reader.h"

Reader::Reader()
{
    bytes = 0;
}

Reader::Reader(string fileName)
{
    bytes = 0;
    const int bufferSize = 1 << 16;
    char buffer[bufferSize];
    file.rdbuf()->pubsetbuf(buffer, bufferSize);
    file.open(fileName, std::ios::in | std::ios::binary);
    file >> std::noskipws;
}

char Reader::readChar()
{
    uint8_t tmp;
    file >> tmp;
    bytes++;
    return tmp;
}

char16_t Reader::readChar16()
{
    char16_t i;
    char* c = (char*)(&i);
    file.read(c, 2);
    bytes += 2;
    return i;
}

uint16_t Reader::readUint16()
{
    uint16_t i;
    char* c = (char*)(&i);
    file.read(c, 2);
    bytes += 2;
    return i;
}

int16_t Reader::readInt16()
{
    int16_t i;
    char* c = (char*)(&i);
    file.read(c, 2);
    bytes += 2;
    return i;
}

uint32_t Reader::readUint32()
{
    uint32_t i;
    char* c = (char*)(&i);
    file.read(c, 4);
    bytes += 4;
    return i;
}

int32_t Reader::readInt32()
{
    int32_t i;
    char* c = (char*)(&i);
    file.read(c, 4);
    bytes += 4;
    return i;
}

uint64_t Reader::readUint64()
{
    uint64_t i;
    char* c = (char*)(&i);
    file.read(c, 8);
    bytes += 8;
    return i;
}

int64_t Reader::readInt64()
{
    int64_t i;
    char* c = (char*)(&i);
    file.read(c, 8);
    bytes += 8;
    return i;
}

float_t Reader::readFloat32()
{
    float_t f;
    char* c = (char*)(&f);
    file.read(c, 4);
    bytes += 4;
    return f;
}

double_t Reader::readFloat64()
{
    double_t f;
    char* c = (char*)(&f);
    file.read(c, 8);
    bytes += 8;
    return f;
}

string Reader::readString(char &length)
{
    string value;
    for (int j = 0; j < length; j++) value += readChar();
    if (value == "Vertices")
    {
        int k;
    }
    return value;
}

string Reader::readString(uint32_t& length)
{
    string value;
    for (int j = 0; j < length; j++) value += readChar();
    if (value == "Vertices")
    {
        int k;
    }
    return value;
}

uint64_t Reader::pos()
{
    return bytes;
}

void Reader::seekg(uint64_t &offset)
{
    file.seekg(offset);
    bytes = offset;
}

bool Reader::is_open()
{
    return file.is_open();
}

void Reader::close()
{
    file.close();
}
