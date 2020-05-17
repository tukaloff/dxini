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

	return obj;
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
		if (node.name == "Type") si.Type = node.propertyList.at(0).raw;
		else if (node.name == "Version") si.Version = node.propertyList.at(0).prim.I32;
		else if (node.name == "MetaData") si.Metadata = readMetadata(node);
		else if (node.name == "Properties70") si.Properties70 = readProperties70(node);
		else throw std::string("Unknown node: \"" + node.name + "\"");
	}

	for (auto node : doc.propertyList)
	{
		throw std::string("Unknown node: \"" + node.typeCode);
	}

	return si;
}

Metadata FBXObjectReader::readMetadata(FBXNode doc)
{
	Metadata md = {};

	for (auto node : doc.nodes)
	{
		if (node.name == "Version") md.Version = node.propertyList.at(0).prim.I32;
		else if (node.name == "Title") md.Title = node.propertyList.at(0).raw;
		else if (node.name == "Subject") md.Subject = node.propertyList.at(0).raw;
		else if (node.name == "Author") md.Author = node.propertyList.at(0).raw;
		else if (node.name == "Keywords") md.Keywords = node.propertyList.at(0).raw;
		else if (node.name == "Revision") md.Revision = node.propertyList.at(0).raw;
		else if (node.name == "Comment") md.Comment = node.propertyList.at(0).raw;
		else throw std::string("Unknown node: \"" + node.name + "\"");
	}

	return md;
}

Properties70 FBXObjectReader::readProperties70(FBXNode doc)
{
	Properties70 p70 = {};

	for (auto node : doc.nodes)
	{
		throw std::string("Unknown node: \"" + node.name + "\"");
	}

	return p70;
}
