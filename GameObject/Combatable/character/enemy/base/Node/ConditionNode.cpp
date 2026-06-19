#include "ConditionNode.h"

NodeStatus ConditionNode::Tick(Blackboard& blackboard)
{
	lastStatus_ = condition(blackboard) ? NodeStatus::Success : NodeStatus::Failure;
	return lastStatus_;
}
