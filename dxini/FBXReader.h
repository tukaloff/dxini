#pragma once
#include <string>
#include <fstream>
#include <vector>
#include "Reader.h"
#include "FBXNode.h"
#include "FBXDocument.h"

using std::ifstream;
using std::string;

class FBXReader
{
public:
	FBXReader();
	FBXReader(string filename);
	FBXDocument read();

	string toJson(FBXNode &doc);
	string toJson(FBXProperty& doc);
private:
	int readNode(uint64_t offset, FBXNode& parent);
	bool isLittleEndian();
	Reader rd;
	string json;
};

