#pragma once
#include <fstream>
#include <string>

using std::ifstream;
using std::string;

class Reader
{
public:
	Reader();
	Reader(string fileName);

	char readChar();
	char16_t readChar16();
	uint16_t readUint16();
	int16_t readInt16();
	uint32_t readUint32();
	int32_t readInt32();
	uint64_t readUint64();
	int64_t readInt64();
	float_t readFloat32();
	double_t readFloat64();
	string readString(char &length);

	uint64_t pos();
	void seekg(uint64_t &offset);
	bool is_open();
	void close();
private:
	uint64_t bytes;
	ifstream file;
};

