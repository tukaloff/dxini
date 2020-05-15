#include "FBXObjectReader.h"

FBXObjectReader::FBXObjectReader()
{
}

FBXObjectReader::FBXObjectReader(FBXDocument& doc)
{
	document = doc;
}

FBXObject FBXObjectReader::read()
{
	return readObject(document);
}

FBXObject FBXObjectReader::readObject(FBXDocument& doc)
{
	FBXObject obj = FBXObject();

	for (auto node : doc.root.nodes)
	{
		if (node.name == "FBXHeaderExtension") 
		{
			obj.FBXHeaderExtension = readHeaderExtention(node);
		}
		else
		{
			throw std::string("Unknown node: \"" + node.name + "\"");
		}
	}

	return FBXObject();
}

FBXHeaderExtension FBXObjectReader::readHeaderExtention(FBXNode doc)
{
	FBXHeaderExtension headerExt = FBXHeaderExtension();

	for (auto node : doc.nodes)
	{
		if (node.name == "FBXHeaderVersion")
		{
			headerExt.FBXHeaderVersion = node.propertyList.at(0).prim.I32;
		}
		else if (node.name == "FBXVersion")
		{
			headerExt.FBXVersion = node.propertyList.at(0).prim.I32;
		}
		else if (node.name == "EncryptionType")
		{
			headerExt.EncryptionType = node.propertyList.at(0).prim.I32;
		}
		else if (node.name == "CreationTimeStamp")
		{
			headerExt.CreationTimeStamp = readCreationTimeStamp(node);
		}
		else if (node.name == "Creator")
		{
			headerExt.Creator = node.propertyList.at(0).raw;
		}
		else if (node.name == "SceneInfo")
		{
			headerExt.SceneInfo = readSceneInfo(node);
		}
		else {
			throw std::string("Unknown node: \"" + node.name + "\"");
		}
	}

	return headerExt;
}

CreationTimeStamp FBXObjectReader::readCreationTimeStamp(FBXNode doc)
{
	CreationTimeStamp timestamp;
	for (auto node : doc.nodes)
	{
		if (node.name == "Version") timestamp.Version = node.propertyList.at(0).prim.I32;
		else if (node.name == "Year") timestamp.Year = node.propertyList.at(0).prim.I32;
		else if (node.name == "Month") timestamp.Month = node.propertyList.at(0).prim.I32;
		else if (node.name == "Day") timestamp.Day = node.propertyList.at(0).prim.I32;
		else if (node.name == "Hour") timestamp.Hour = node.propertyList.at(0).prim.I32;
		else if (node.name == "Minute") timestamp.Minute = node.propertyList.at(0).prim.I32;
		else if (node.name == "Second") timestamp.Second = node.propertyList.at(0).prim.I32;
		else if (node.name == "Millisecond") timestamp.Millisecond = node.propertyList.at(0).prim.I32;
		else throw std::string("Unknown node: \"" + node.name + "\"");
	}
	return timestamp;
}

SceneInfo FBXObjectReader::readSceneInfo(FBXNode doc)
{
	SceneInfo si;

	for (auto node : doc.nodes)
	{
		throw std::string("Unknown node: \"" + node.name + "\"");
	}

	for (auto node : doc.propertyList)
	{
		throw std::string("Unknown node: \"" + node.typeCode);
	}

	return si;
}
