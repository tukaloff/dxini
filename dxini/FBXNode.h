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
	vector<FBXNode> nodes;
	void addNode(FBXNode node);
private:
};

