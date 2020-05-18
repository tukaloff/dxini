#pragma once
#include "FBXHeaderExtension.h"
#include "FBXNode.h"

struct RAW {
	string raw;
};

class FBXObject
{
public:
	FBXHeaderExtension FBXHeaderExtension;
	RAW FileId;
	string CreationTime;
	string Creator;

	FBXNode raw;
};

