#pragma once
#include <string>
#include <vector>
#include <fstream>

using namespace std;

struct PAIR {
	string key;
	string value;
};

class Properties
{
public:
	Properties();
	Properties(string* path);
	void load();
	string get(string* key);
private:
	string path;
	vector<PAIR> properties;
};

