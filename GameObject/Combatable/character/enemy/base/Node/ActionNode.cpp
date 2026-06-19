#include "ActionNode.h"

NodeStatus ActionNode::Tick(Blackboard& blackboard)
{
	lastStatus_ = action(blackboard);
	return lastStatus_;
}
