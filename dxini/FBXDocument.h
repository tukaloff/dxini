#pragma once
#include "FBXNode.h"

struct HEADER {
	string name;
	char bb[2];
	uint32_t version;
};

class FBXDocument
{
public:
	HEADER header;
	FBXNode root;
};

