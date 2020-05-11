#include "FBXReader.h"

void FBXReader::set_filename(string file)
{
	filename = file;
}

void FBXReader::read()
{
	const int bufferSize = 1 << 16;
	char buffer[bufferSize];
	file.rdbuf()->pubsetbuf(buffer, bufferSize);
	file.open(filename, std::ios::in | std::ios::binary);
	if (file.is_open()) {
		read(file);
	}
	else {
		throw std::string("Cannot read from file: \"" + filename + "\"");
	}
	file.close();
}

void FBXReader::read(std::ifstream& input)
{
	HEADER hd;
	input >> std::noskipws;
	uint8_t charTmp;
	uint32_t intTmp;
	char* c = (char*)(&intTmp);
	int offset = 27;
	int bytes = 0;

	string actual;

	// Kaydara FBX Binary  \x00
	for (int i = 0; i < sizeof(hd.name); i++)
	{
		hd.name[i] = getc();
		bytes++;
	}

	// [0x1A, 0x00]
	for (int i = 0; i < sizeof(hd.bb); i++)
	{
		hd.bb[i] = getc();
		bytes++;
	}

	// version (7300)
	hd.version = readUint32();
	bytes += 4;

	while(true)
	{
		if (readNode(offset, bytes, nodes) == 0) break;
	}

	return;
}

int FBXReader::readNode(int &offset, int &bytes, std::vector<NODE> &nodes)
{
	NODE node;
	file.seekg(offset);
	node.endOffset = readUint32();
 	bytes += 4;

	node.numProperties = readUint32();
	bytes += 4;
	node.propertyListLen = readUint32();
	bytes += 4;
	node.nameLen = getc();
	bytes++;
	for (int j = 0; j < node.nameLen; j++) node.name += getc();
	bytes += node.nameLen;

	for (int j = 0; j < node.numProperties; j++)
	{
		PROPERTY prop;
		prop.typeCode = getc();
		bytes += 1;

		if (prop.typeCode == 'S' || prop.typeCode == 'R')
		{
			prop.lenght = readUint32();
			bytes += 4;
			for (int k = 0; k < prop.lenght; k++) prop.raw.push_back(getc());
			bytes += prop.lenght;
		}
		else if (prop.typeCode == 'Y')
		{
			prop.propertyValue.I16 = readInt16();
			bytes += 2;
		}
		else if (prop.typeCode == 'C')
		{
			prop.propertyValue.B = getc();
			bytes++;
		}
		else if (prop.typeCode == 'I')
		{
			prop.propertyValue.I32 = readUint32();
			bytes += 4;
		}
		else if (prop.typeCode == 'F')
		{
			prop.propertyValue.F32 = readFloat();
			bytes += 4;
		}
		else if (prop.typeCode == 'D')
		{
			prop.propertyValue.F64 = readDouble();
			bytes += 8;
		}
		else if (prop.typeCode == 'L') 
		{
			prop.propertyValue.I64 = readUint64();
			bytes += 8;
		}
		else
		{
			return 0;
		}

		node.propertyList.push_back(prop);
	}

	if (node.endOffset == 0) return 0;

	while (bytes < node.endOffset) {
		readNode(bytes, bytes, node.nodes);
	}
	//bytes += 13;

	nodes.push_back(node);
	offset = node.endOffset;

	return node.endOffset;
}

uint32_t FBXReader::readUint32()
{
	uint32_t i;
	char* c = (char*)(&i);
	file.read(c, 4);
	return i;
}

float FBXReader::readFloat()
{
	float f;
	char* c = (char*)(&f);
	file.read(c, 4);
	return f;
}

int16_t FBXReader::readInt16()
{
	int16_t i;
	char* c = (char*)(&i);
	file.read(c, 2);
	return i;
}

double FBXReader::readDouble()
{
	double f;
	char* c = (char*)(&f);
	file.read(c, 8);
	return f;
}

uint64_t FBXReader::readUint64()
{
	uint64_t i;
	char* c = (char*)(&i);
	file.read(c, 8);
	return i;
}

uint8_t FBXReader::getc()
{
	uint8_t tmp;
	file >> tmp;
	return tmp;
}

bool FBXReader::isLittleEndian()
{
	uint16_t number = 0x1;
	char* numPtr = (char*)&number;
	return (numPtr[0] == 1);
}