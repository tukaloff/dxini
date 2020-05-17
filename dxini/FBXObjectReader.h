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
	Metadata readMetadata(FBXNode doc);
	Properties70 readProperties70(FBXNode doc);
};

