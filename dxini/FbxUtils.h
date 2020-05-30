#pragma once
#include <fbxsdk.h>
#include <fstream>

class FbxUtils
{
public:
	FbxUtils();
	static FbxUtils* Init();
	void test();
private:
	int numTabs = 0;
	void PrintTabs();
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType type);
	void PrintAttribute(FbxNodeAttribute* pAttribute);
	void PrintNode(FbxNode* pNode);
};

