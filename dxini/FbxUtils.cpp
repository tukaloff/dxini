#include "FbxUtils.h"

std::wofstream out;

FbxUtils::FbxUtils()
{
		
}

FbxUtils* FbxUtils::Init()
{
	FbxUtils fbx = FbxUtils();
	FbxUtils* pFbx = &fbx;
	return pFbx;
}

void FbxUtils::test()
{
	FbxManager* lSdkManager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
	lImporter->Initialize("model/Flat2.fbx", -1, lSdkManager->GetIOSettings());

	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");
	lImporter->Import(lScene);

	lImporter->Destroy();
	out.open("out.xml", std::ios_base::out);

	FbxNode* lRootNode = lScene->GetRootNode();
	if (lRootNode)
	{
		for (int i = 0; i < lRootNode->GetChildCount(); i++)
		{
			PrintNode(lRootNode->GetChild(i));
		}
	}

	out.close();

	lSdkManager->Destroy();
}

void FbxUtils::PrintTabs() {
	for (int i = 0; i < numTabs; i++)
		out << "\t";
}

FbxString FbxUtils::GetAttributeTypeName(FbxNodeAttribute::EType type)
{
	switch (type) {
	case FbxNodeAttribute::eUnknown: return "unidentified";
	case FbxNodeAttribute::eNull: return "null";
	case FbxNodeAttribute::eMarker: return "marker";
	case FbxNodeAttribute::eSkeleton: return "skeleton";
	case FbxNodeAttribute::eMesh: return "mesh";
	case FbxNodeAttribute::eNurbs: return "nurbs";
	case FbxNodeAttribute::ePatch: return "patch";
	case FbxNodeAttribute::eCamera: return "camera";
	case FbxNodeAttribute::eCameraStereo: return "stereo";
	case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
	case FbxNodeAttribute::eLight: return "light";
	case FbxNodeAttribute::eOpticalReference: return "optical reference";
	case FbxNodeAttribute::eOpticalMarker: return "marker";
	case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
	case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
	case FbxNodeAttribute::eBoundary: return "boundary";
	case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
	case FbxNodeAttribute::eShape: return "shape";
	case FbxNodeAttribute::eLODGroup: return "lodgroup";
	case FbxNodeAttribute::eSubDiv: return "subdiv";
	default: return "unknown";
	}
}

void FbxUtils::PrintAttribute(FbxNodeAttribute* pAttribute)
{
	if (!pAttribute) return;

	FbxString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
	FbxString attrName = pAttribute->GetName();
	PrintTabs();
	// Note: to retrieve the character array of a FbxString, use its Buffer() method.
	out << "<attribute type='" << typeName.Buffer() << "' name='" << attrName.Buffer() << "'/>\n";
}

void FbxUtils::PrintNode(FbxNode* pNode)
{
	PrintTabs();
	const char* nodeName = pNode->GetName();
	FbxDouble3 translation = pNode->LclTranslation.Get();
	FbxDouble3 rotation = pNode->LclRotation.Get();
	FbxDouble3 scaling = pNode->LclScaling.Get();

	// Print the contents of the node.
	out << "<node name='" << nodeName << "' translation='(" << translation[0] << ", " << translation[1] << ", " << translation[2] << ")' "
		<< "rotation='(" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << ")' "
		<< "scaling='(" << scaling[0] << ", " << scaling[1] << ", " << scaling[2] << ")'>\n";
	numTabs++;

	// Print the node's attributes.
	for (int i = 0; i < pNode->GetNodeAttributeCount(); i++)
		PrintAttribute(pNode->GetNodeAttributeByIndex(i));

	// Recursively print the children.
	for (int j = 0; j < pNode->GetChildCount(); j++)
		PrintNode(pNode->GetChild(j));

	numTabs--;
	PrintTabs();
	out << "</node>\n";
}
