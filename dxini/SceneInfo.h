#pragma once
#include <string>

using std::string;

struct Metadata {
	uint32_t Version;
	string Title;
	string Subject;
	string Author;
	string Keywords;
	string Revision;
	string Comment;
};

struct Properties70 {

};

class SceneInfo
{
public:
	string Type;
	uint32_t Version;
	Metadata Metadata;
	Properties70 Properties70;
};

