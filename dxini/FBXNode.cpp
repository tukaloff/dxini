#include "FBXNode.h"

FBXNode::FBXNode()
{
}

void FBXNode::addNode(FBXNode node)
{
	nodes.push_back(node);
}
