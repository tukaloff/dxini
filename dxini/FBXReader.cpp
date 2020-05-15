#include "FBXReader.h"
#include "FBXDocument.h"

FBXReader::FBXReader()
{
	json = "";
}

FBXReader::FBXReader(string filename)
{
	json = "";
	rd = Reader(filename);
	if (!rd.is_open()) 
	{
		throw std::string("Cannot read from file: \"" + filename + "\"");
	}
}

FBXDocument FBXReader::read()
{
	FBXDocument doc;
	uint8_t charTmp;
	uint32_t intTmp;
	char* c = (char*)(&intTmp);
	int offset = 27;

	string actual;

	// Kaydara FBX Binary  \x00
	char namelen = 21;
	doc.header.name = rd.readString(namelen);

	// [0x1A, 0x00]
	for (int i = 0; i < sizeof(doc.header.bb); i++)
	{
		doc.header.bb[i] = rd.readChar();
	}

	// version (7300)
	doc.header.version = rd.readUint32();

	while(true)
	{
		if (readNode(rd.pos(), doc.root) == 0) break;
	}

	rd.close();

	return doc;
}

string FBXReader::toJson(FBXNode &doc)
{
	string allJson = "[";
	for (int i = 0; i < doc.nodes.size(); i++)
	{
		FBXNode node = doc.nodes.at(i);
		string nodeJson = "";
		nodeJson += "{\"name\": \"" + node.name + "\",";

		nodeJson += "\"propertyList\": "; 
		nodeJson += std::to_string(node.propertyList.size());
		nodeJson += ",";
		//for (int j = 0; j < node.propertyList.size(); j++)
		//{
		//	FBXProperty prop = node.propertyList.at(i);
		//	nodeJson += "{";
		//	nodeJson += "{\"typeCode\": \"" + prop.typeCode;
		//	nodeJson += "\",";
		//	nodeJson += "{\"raw\": \"" + prop.raw;
		//	nodeJson += "\",";
		//	nodeJson += "{\"encoded\": \"" + prop.encoded;
		//	nodeJson += "\",";
		//	/*nodeJson += "{\"prim\": \"" + prop.prim;
		//	nodeJson += "\",";*/
		//	nodeJson += "}";
		//	if (i != node.propertyList.size() - 1)
		//		nodeJson += ",";
		//}
		//nodeJson += "]";

		nodeJson += "\"nodes\": " + toJson(node) + "}";

		if (i != doc.nodes.size() - 1)
			nodeJson += ",";
		allJson += nodeJson;
	}
	allJson += "]";
	return allJson;
}

string FBXReader::toJson(FBXProperty& doc)
{
	return string();
}

int FBXReader::readNode(uint64_t offset, FBXNode& parent)
{
	FBXNode node;
	rd.seekg(offset);
	uint32_t endOffset = rd.readUint32();

	uint32_t numProperties = rd.readUint32();
	uint32_t propertyListLen = rd.readUint32();
	char nameLen = rd.readChar();
	node.name += rd.readString(nameLen);

	for (int j = 0; j < numProperties; j++)
	{
		FBXProperty prop;
		prop.typeCode = rd.readChar();

		if (prop.typeCode == 'S' || prop.typeCode == 'R')
		{
			prop.lenght = rd.readUint32();
			prop.raw = rd.readString(prop.lenght);
		}
		else if (prop.typeCode == 'Y')
		{
			prop.prim.I16 = rd.readInt16();
		}
		else if (prop.typeCode == 'C')
		{
			prop.prim.B8 = rd.readChar();
		}
		else if (prop.typeCode == 'I')
		{
			prop.prim.I32 = rd.readInt32();
		}
		else if (prop.typeCode == 'F')
		{
			prop.prim.F32 = rd.readFloat32();
		}
		else if (prop.typeCode == 'D')
		{
			prop.prim.F64 = rd.readFloat64();
		}
		else if (prop.typeCode == 'L') 
		{
			prop.prim.I64 = rd.readInt64();
		}
		else if (prop.typeCode == 'f')
		{
			uint32_t arrlength = rd.readUint32();
			uint32_t encoding = rd.readUint32();
			uint32_t compressedLength = rd.readUint32();
			if (encoding == 1)
			{
				prop.encoded = rd.readString(compressedLength);
			}
			else
			{
				for (int k = 0; k < arrlength; k++) 
				{
					PRIMITIVE pr;
					pr.F32 = rd.readFloat32();
					prop.arr.push_back(pr);
				}
			}
		}
		else if (prop.typeCode == 'd')
		{
			uint32_t arrlength = rd.readUint32();
			uint32_t encoding = rd.readUint32();
			uint32_t compressedLength = rd.readUint32();
			if (encoding == 1)
			{
				prop.encoded = rd.readString(compressedLength);
			}
			else
			{
				for (int k = 0; k < arrlength; k++)
				{
					PRIMITIVE pr;
					pr.F64 = rd.readFloat64();
					prop.arr.push_back(pr);
				}
			}
		}
		else if (prop.typeCode == 'l')
		{
			uint32_t arrlength = rd.readUint32();
			uint32_t encoding = rd.readUint32();
			uint32_t compressedLength = rd.readUint32();
			if (encoding == 1)
			{
				prop.encoded = rd.readString(compressedLength);
			}
			else
			{
				for (int k = 0; k < arrlength; k++)
				{
					PRIMITIVE pr;
					pr.I64 = rd.readInt64();
					prop.arr.push_back(pr);
				}
			}
		}
		else if (prop.typeCode == 'i')
		{
			uint32_t arrlength = rd.readUint32();
			uint32_t encoding = rd.readUint32();
			uint32_t compressedLength = rd.readUint32();
			if (encoding == 1)
			{
				prop.encoded = rd.readString(compressedLength);
			}
			else
			{
				for (int k = 0; k < arrlength; k++)
				{
					PRIMITIVE pr;
					pr.I32 = rd.readInt32();
					prop.arr.push_back(pr);
				}
			}
		}
		else if (prop.typeCode == 'b')
		{
			uint32_t arrlength = rd.readUint32();
			uint32_t encoding = rd.readUint32();
			uint32_t compressedLength = rd.readUint32();
			if (encoding == 1)
			{
				prop.encoded = rd.readString(compressedLength);
			}
			else
			{
				for (int k = 0; k < arrlength; k++)
				{
					PRIMITIVE pr;
					pr.B8 = rd.readChar();
					prop.arr.push_back(pr);
				}
			}
		}
		else
		{
			throw std::string("Unknown property type: " + prop.typeCode);
		}

		node.propertyList.push_back(prop);
	}

	if (endOffset == 0) return 0;

	while (rd.pos() < endOffset) {
		readNode(rd.pos(), node);
	}

	parent.addNode(node);

	return endOffset;
}

bool FBXReader::isLittleEndian()
{
	uint16_t number = 0x1;
	char* numPtr = (char*)&number;
	return (numPtr[0] == 1);
}