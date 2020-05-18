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

struct Compound {
	// unknown
};

struct Properties70 {
	string DocumentUrl;
	string SrcDocumentUrl;
	Compound Original;
	string Original_ApplicationVendor;
	string Original_ApplicationName;
	string Original_ApplicationVersion;
	string Original_DateTime_GMT; // need date "01/01/1970 00:00:00.000"
	string Original_FileName;
	Compound LastSaved;
	string LastSaved_ApplicationVendor;
	string LastSaved_ApplicationName;
	string LastSaved_ApplicationVersion;
	string LastSaved_DateTime_GMT; // need date
};

class SceneInfo
{
public:
	string Type;
	uint32_t Version;
	Metadata Metadata;
	Properties70 Properties70;
};

