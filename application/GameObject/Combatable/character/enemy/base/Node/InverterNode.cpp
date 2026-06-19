#include "InverterNode.h"

NodeStatus InverterNode::Tick(Blackboard& blackboard)
{
    NodeStatus status = child_->Tick(blackboard);
    switch (status)
    {
    case NodeStatus::Success:
        return NodeStatus::Failure;
    case NodeStatus::Failure:
        return NodeStatus::Success;
    case NodeStatus::Running:
    default:
        return status;
    }
}

void InverterNode::Reset()
{
    if (child_)
    {
        child_->Reset();
    }
}
