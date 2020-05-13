#pragma once
#include <vector>
#include <string>
#include "FBXPrimitive.h"

using std::vector;
using std::string;

struct ARRAY {
	uint32_t length;
	uint32_t encoding;
	uint32_t compressedLength;

	string encoded;

	vector<float> AF32;
	vector<double> AF64;
	vector<int32_t> AI32;
	vector<int64_t> AI64;
	vector<bool> AB8;
};

class FBXProperty
{
public:
	char typeCode;
	uint32_t lenght;
	string raw;

	FBXPrimitive prim;
	ARRAY arr;
};

