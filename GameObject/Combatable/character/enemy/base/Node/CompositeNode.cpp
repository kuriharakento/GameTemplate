#include "CompositeNode.h"

void CompositeNode::AddChild(std::unique_ptr<BTNode> child)
{
	children.push_back(std::move(child));
}

void CompositeNode::Reset()
{
	currentIndex = 0;
	for (auto& child : children)
	{
		child->Reset();
	}
}
