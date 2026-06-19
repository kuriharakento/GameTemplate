#include "ParallelNode.h"

NodeStatus ParallelNode::Tick(Blackboard& blackboard)
{
	size_t successCount = 0;
	size_t failureCount = 0;

	for (auto& child : children)
	{
		NodeStatus status = child->Tick(blackboard);
		if (status == NodeStatus::Success)
			++successCount;
		else if (status == NodeStatus::Failure)
			++failureCount;
	}

	if (successCount >= successThreshold_)
	{
		return NodeStatus::Success;
	}
	if (failureCount >= failureThreshold_)
	{
		return NodeStatus::Failure;
	}
	return NodeStatus::Running;
}

void ParallelNode::Reset()
{
	for (auto& child : children)
	{
		child->Reset();
	}
}
