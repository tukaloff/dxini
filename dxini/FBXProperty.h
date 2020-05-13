#pragma once
#include <vector>
#include <string>

using std::vector;
using std::string;

union PRIMITIVE {
	double F64;
	int64_t I64;
	int32_t I32;
	float F32;
	int16_t I16;
	bool B8;
};

class FBXProperty
{
public:
	char typeCode;
	uint32_t lenght;
	string raw;

	string encoded;

	PRIMITIVE prim;
	vector<PRIMITIVE> arr;
};

