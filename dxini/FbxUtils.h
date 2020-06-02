#pragma once
#include <fbxsdk.h>
#include <fstream>
#include <string>

using namespace std;

class FbxUtils
{
public:
	FbxUtils();
	FbxUtils(const char* path);
	void test(string* outPath);
private:
	const char* fileName;
	int numTabs = 0;
	void PrintTabs();
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType type);
	void PrintAttribute(FbxNodeAttribute* pAttribute);
	void PrintNode(FbxNode* pNode);
};

