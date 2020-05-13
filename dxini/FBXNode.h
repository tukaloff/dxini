#pragma once
#include <vector>
#include <string>
#include "FBXProperty.h"

using std::vector;
using std::string;

class FBXNode
{
public:
	FBXNode();

	string name;
	vector<FBXProperty> propertyList;
	void addNode(FBXNode node);
private:
	vector<FBXNode> nodes;
};

