#pragma once
#include <string>
#include <fstream>
#include <vector>

using std::ifstream;
using std::string;

struct HEADER {
	char name[21];
	char bb[2];
	uint32_t version;
};

struct PROPERTY_VALUE {
	uint16_t I16;
	bool B;
	uint32_t I32;
	float F32;
	double F64;
	int64_t I64;
};

struct PROPERTY {
	char typeCode;
	int lenght;
	std::vector<uint8_t> raw;
	PROPERTY_VALUE propertyValue;
};

struct NODE {
	uint32_t endOffset;
	uint32_t numProperties;
	uint32_t propertyListLen;
	char nameLen;
	string name;
	std::vector<PROPERTY> propertyList;
	std::vector<NODE> nodes;
};

class FBXReader
{
public:
	void set_filename(std::string filename);
	void read();
private:
	void read(std::ifstream& input);
	int readNode(int& offset, int& bytes, std::vector<NODE>& nodes);
	uint32_t readUint32();
	float readFloat();
	int16_t readInt16();
	double readDouble();
	uint64_t readUint64();
	uint8_t getc();
	bool isLittleEndian();
	std::string filename;
	std::ifstream file;

	std::vector<NODE> nodes;
};

