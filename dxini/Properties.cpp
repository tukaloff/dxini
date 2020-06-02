#include "Properties.h"
#include <iostream>
#include <sstream>

Properties::Properties()
{
	path = "application.properties";
}

Properties::Properties(string* path)
{
	this->path = *path;
}

void Properties::load()
{
	ifstream ifs(path);

	string line;
	string key;
	string value;

	while (getline(ifs, line)) {

		istringstream wiss(line);
		if (getline(wiss, key, '='))
		{
			if (key[0] == '#') 
				continue;
			if (getline(wiss, value))
				this->properties.push_back({ key, value });
		}

	}

	ifs.close();
}

string Properties::get(string* key)
{
	for (PAIR in : properties)
	{
		if (in.key == *key) {
			return in.value;
		}
	}
	return nullptr;
}
