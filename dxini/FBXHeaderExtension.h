#pragma once
#include <vector>
#include <string>
#include "CreationTimeStamp.h"
#include "SceneInfo.h"

using std::string;

class FBXHeaderExtension
{
public:
	uint32_t FBXHeaderVersion;
	uint32_t FBXVersion;
	uint32_t EncryptionType;
	CreationTimeStamp CreationTimeStamp;
	string Creator;
	SceneInfo SceneInfo;
};

