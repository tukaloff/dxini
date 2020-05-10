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

struct PROPERTY {
	char typeCode;
	int lenght;
	std::vector<uint8_t> raw;
};

struct NODE {
	uint32_t endOffset;
	uint32_t numProperties;
	uint32_t propertyListLen;
	char nameLen;
	string name;
	std::vector<PROPERTY> propertyList;
};

class FBXReader
{
public:
	void set_filename(std::string filename);
	void read();
private:
	void read(std::ifstream& input);
	uint32_t readUint32();
	uint8_t getc();
	bool isLittleEndian();
	std::string filename;
	std::ifstream file;

	std::vector<NODE> nodes;
};

