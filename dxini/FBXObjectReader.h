#pragma once
#include "FBXDocument.h"
#include "FBXObject.h"
#include "SceneInfo.h"
class FBXObjectReader
{
public:
	FBXObjectReader();
	FBXObjectReader(FBXDocument& doc);
	FBXObject read();

private:
	FBXDocument document;

	FBXObject readObject(FBXDocument& doc);
	FBXHeaderExtension readHeaderExtention(FBXNode doc);
	CreationTimeStamp readCreationTimeStamp(FBXNode doc);
	SceneInfo readSceneInfo(FBXNode doc);
};

