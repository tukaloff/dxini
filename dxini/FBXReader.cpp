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

	string actual;

	// Kaydara FBX Binary  \x00
	for (int i = 0; i < sizeof(hd.name); i++)
	{
		hd.name[i] = getc();
	}

	// [0x1A, 0x00]
	for (int i = 0; i < sizeof(hd.bb); i++)
	{
		hd.bb[i] = getc();
	}

	// version (7300)
	hd.version = readUint32();

	file.seekg(offset);

	while(true)
	{
		NODE node;
		file.seekg(offset);
		node.endOffset = readUint32();

		if (node.endOffset == 0) break;

		node.numProperties = readUint32();
		node.propertyListLen = readUint32();
		node.nameLen = getc();
		for (int j = 0; j < node.nameLen; j++) node.name += getc();

		for (int j = 0; j < node.numProperties; j++)
		{
			PROPERTY prop;
			prop.typeCode = getc();

			if (prop.typeCode == 'S' || prop.typeCode == 'R')
			{
				prop.lenght = readUint32();
				for (int k = 0; k < prop.lenght; k++) prop.raw.push_back(getc());
			}

			switch (prop.typeCode)
			{
			default:
			{
				break;
			}
			}
			if (prop.typeCode == '¼') break;
			node.propertyList.push_back(prop);
		}

		nodes.push_back(node);
		offset = node.endOffset;
	}

	return;
}

uint32_t FBXReader::readUint32()
{
	uint32_t i;
	char* c = (char*)(&i);
	if (isLittleEndian()) {
		file.read(c, 4);
	}
	else {
		c[3] = getc();
		c[2] = getc();
		c[1] = getc();
		c[0] = getc();
	}
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