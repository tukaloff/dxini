#pragma once
#include <string>

using std::string;

class PropertiesLoader
{
public:
	PropertiesLoader();
	PropertiesLoader(string* path);
private:
	string propertiesPath;
};

