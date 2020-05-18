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
		if (node.name == "FBXHeaderExtension") obj.FBXHeaderExtension = readHeaderExtention(node);
		else if (node.name == "FileId") obj.FileId = { node.propertyList.at(0).raw };
		else if (node.name == "CreationTime") obj.CreationTime = node.propertyList.at(0).raw;
		else if (node.name == "Creator") obj.Creator = node.propertyList.at(0).raw;
		else throw std::string("Unknown node: \"" + node.name + "\"");
	}

	return obj;
}

FBXHeaderExtension FBXObjectReader::readHeaderExtention(FBXNode doc)
{
	FBXHeaderExtension headerExt = FBXHeaderExtension();

	for (auto node : doc.nodes)
	{
		if (node.name == "FBXHeaderVersion") headerExt.FBXHeaderVersion = node.propertyList.at(0).prim.I32;
		else if (node.name == "FBXVersion") headerExt.FBXVersion = node.propertyList.at(0).prim.I32;
		else if (node.name == "EncryptionType") headerExt.EncryptionType = node.propertyList.at(0).prim.I32;
		else if (node.name == "CreationTimeStamp") headerExt.CreationTimeStamp = readCreationTimeStamp(node);
		else if (node.name == "Creator") headerExt.Creator = node.propertyList.at(0).raw;
		else if (node.name == "SceneInfo") headerExt.SceneInfo = readSceneInfo(node);
		else throw std::string("Unknown node: \"" + node.name + "\"");
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
		if (node.raw == "GlobalInfo" || node.lenght == 21) {}
		else if (node.raw == "UserData") {}
		else throw std::string("Unknown node: \"" + node.typeCode);
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
		if (node.name == "P")
		{
			if (node.propertyList.at(0).raw == "DocumentUrl") p70.DocumentUrl = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "SrcDocumentUrl") p70.SrcDocumentUrl = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "Original") p70.Original = readCompound(node);
			else if (node.propertyList.at(0).raw == "Original|ApplicationVendor") p70.Original_ApplicationVendor = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "Original|ApplicationName") p70.Original_ApplicationName = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "Original|ApplicationVersion") p70.Original_ApplicationVersion = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "Original|DateTime_GMT") p70.Original_DateTime_GMT = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "Original|FileName") p70.Original_FileName = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "LastSaved") p70.LastSaved = readCompound(node);
			else if (node.propertyList.at(0).raw == "LastSaved|ApplicationVendor") p70.LastSaved_ApplicationVendor = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "LastSaved|ApplicationName") p70.LastSaved_ApplicationName = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "LastSaved|ApplicationVersion") p70.LastSaved_ApplicationVersion = node.propertyList.at(4).raw;
			else if (node.propertyList.at(0).raw == "LastSaved|DateTime_GMT") p70.LastSaved_DateTime_GMT = node.propertyList.at(4).raw;
			else throw std::string("Unknown node: \"" + node.name + "\"");
		}
		else throw std::string("Unknown node: \"" + node.name + "\"");
	}

	return p70;
}

Compound FBXObjectReader::readCompound(FBXNode doc)
{
	if (doc.propertyList.size() > 4) throw std::string("Unknown node: \"" + doc.name + "\"");
	return Compound();
}
