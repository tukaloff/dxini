#pragma once
#include <string>
#include <fstream>
#include <vector>
#include "Reader.h"

using std::ifstream;
using std::string;

struct HEADER {
	char name[21];
	char bb[2];
	uint32_t version;
};

struct PRIMITIVE {
	double F64;
	int64_t I64;
	int32_t I32;
	float F32;
	int16_t I16;
	bool B8;
};

struct ARRAY {
	uint32_t length;
	uint32_t encoding;
	uint32_t compressedLength;

	std::vector<float> AF32;
	std::vector<double> AF64;
	std::vector<int32_t> AI32;
	std::vector<int64_t> AI64;
	std::vector<bool> AB8;
};

struct PROPERTY_VALUE {
	PRIMITIVE prim;
	ARRAY arr;
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
	FBXReader();
	FBXReader(string filename);
	void read();
private:
	int readNode(uint64_t offset, std::vector<NODE>& nodes);
	bool isLittleEndian();
	Reader rd;

	std::vector<NODE> nodes;
};

