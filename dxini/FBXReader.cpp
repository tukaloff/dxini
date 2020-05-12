#include "FBXReader.h"

FBXReader::FBXReader()
{
}

FBXReader::FBXReader(string filename)
{
	rd = Reader(filename);
	if (!rd.is_open()) 
	{
		throw std::string("Cannot read from file: \"" + filename + "\"");
	}
}

void FBXReader::read()
{
	HEADER hd;
	uint8_t charTmp;
	uint32_t intTmp;
	char* c = (char*)(&intTmp);
	int offset = 27;

	string actual;

	// Kaydara FBX Binary  \x00
	for (int i = 0; i < sizeof(hd.name); i++)
	{
		hd.name[i] = rd.readChar();
	}

	// [0x1A, 0x00]
	for (int i = 0; i < sizeof(hd.bb); i++)
	{
		hd.bb[i] = rd.readChar();
	}

	// version (7300)
	hd.version = rd.readUint32();

	while(true)
	{
		if (readNode(rd.pos(), nodes) == 0) break;
	}


	rd.close();

	return;
}

int FBXReader::readNode(uint64_t offset, std::vector<NODE> &nodes)
{
	if (rd.pos() >= 64040)
	{
		int q = 0;
		q++;
	}
	NODE node;
	rd.seekg(offset);
	node.endOffset = rd.readUint32();

	node.numProperties = rd.readUint32();
	node.propertyListLen = rd.readUint32();
	node.nameLen = rd.readChar();
	node.name += rd.readString(node.nameLen);

	if (node.name == "LayerElementNormal")
	{
		int q = 0;
		q++;
	}

	for (int j = 0; j < node.numProperties; j++)
	{
		if (rd.pos() >= 64040)
		{
			int q = 0;
			q++;
		}
		PROPERTY prop;
		prop.typeCode = rd.readChar();

		if (prop.typeCode == 'S' || prop.typeCode == 'R')
		{
			prop.lenght = rd.readUint32();
			for (int k = 0; k < prop.lenght; k++) prop.raw.push_back(rd.readChar());
		}
		else if (prop.typeCode == 'Y')
		{
			prop.propertyValue.prim.I16 = rd.readInt16();
		}
		else if (prop.typeCode == 'C')
		{
			prop.propertyValue.prim.B8 = rd.readChar();
		}
		else if (prop.typeCode == 'I')
		{
			prop.propertyValue.prim.I32 = rd.readInt32();
		}
		else if (prop.typeCode == 'F')
		{
			prop.propertyValue.prim.F32 = rd.readFloat32();
		}
		else if (prop.typeCode == 'D')
		{
			prop.propertyValue.prim.F64 = rd.readFloat64();
		}
		else if (prop.typeCode == 'L') 
		{
			prop.propertyValue.prim.I64 = rd.readInt64();
		}
		else if (prop.typeCode == 'f')
		{
			prop.propertyValue.arr.length = rd.readUint32();
			prop.propertyValue.arr.encoding = rd.readUint32();
			prop.propertyValue.arr.compressedLength = rd.readUint32();
			if (prop.propertyValue.arr.encoding == 1)
			{
				prop.propertyValue.arr.encoded = rd.readString(prop.propertyValue.arr.compressedLength);
			}
			else
			{
				for (int k = 0; k < prop.propertyValue.arr.length; k++) prop.propertyValue.arr.AF32.push_back(rd.readFloat32());
			}
		}
		else if (prop.typeCode == 'd')
		{
			prop.propertyValue.arr.length = rd.readUint32();
			prop.propertyValue.arr.encoding = rd.readUint32();
			prop.propertyValue.arr.compressedLength = rd.readUint32();
			if (prop.propertyValue.arr.encoding == 1)
			{
				prop.propertyValue.arr.encoded = rd.readString(prop.propertyValue.arr.compressedLength);
			}
			else
			{
				for (int k = 0; k < prop.propertyValue.arr.length; k++) prop.propertyValue.arr.AF64.push_back(rd.readFloat64());
			}
		}
		else if (prop.typeCode == 'l')
		{
			prop.propertyValue.arr.length = rd.readUint32();
			prop.propertyValue.arr.encoding = rd.readUint32();
			prop.propertyValue.arr.compressedLength = rd.readUint32();
			if (prop.propertyValue.arr.encoding == 1)
			{
				prop.propertyValue.arr.encoded = rd.readString(prop.propertyValue.arr.compressedLength);
			}
			else
			{
				for (int k = 0; k < prop.propertyValue.arr.length; k++) prop.propertyValue.arr.AI64.push_back(rd.readInt64());
			}
		}
		else if (prop.typeCode == 'i')
		{
			prop.propertyValue.arr.length = rd.readUint32();
			prop.propertyValue.arr.encoding = rd.readUint32();
			prop.propertyValue.arr.compressedLength = rd.readUint32();
			if (prop.propertyValue.arr.encoding == 1)
			{
				prop.propertyValue.arr.encoded = rd.readString(prop.propertyValue.arr.compressedLength);
			}
			else
			{
				for (int k = 0; k < prop.propertyValue.arr.length; k++) prop.propertyValue.arr.AI32.push_back(rd.readInt32());
			}
		}
		else if (prop.typeCode == 'b')
		{
			prop.propertyValue.arr.length = rd.readUint32();
			prop.propertyValue.arr.encoding = rd.readUint32();
			prop.propertyValue.arr.compressedLength = rd.readUint32();
			if (prop.propertyValue.arr.encoding == 1)
			{
				prop.propertyValue.arr.encoded = rd.readString(prop.propertyValue.arr.compressedLength);
			}
			else
			{
				for (int k = 0; k < prop.propertyValue.arr.length; k++) prop.propertyValue.arr.AB8.push_back(rd.readChar());
			}
		}
		else
		{
			throw std::string("Unknown property type: " + prop.typeCode);
		}

		node.propertyList.push_back(prop);
	}

	if (node.endOffset == 0) return 0;

	while (rd.pos() < node.endOffset) {
		readNode(rd.pos(), node.nodes);
	}

	nodes.push_back(node);

	return node.endOffset;
}

bool FBXReader::isLittleEndian()
{
	uint16_t number = 0x1;
	char* numPtr = (char*)&number;
	return (numPtr[0] == 1);
}