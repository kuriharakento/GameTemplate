#include "SequenceNode.h"

NodeStatus SequenceNode::Tick(Blackboard& blackboard)
{
    while (currentIndex < children.size())
    {
        NodeStatus status = children[currentIndex]->Tick(blackboard);
        if (status == NodeStatus::Failure)
        {
            Reset();
            lastStatus_ = NodeStatus::Failure;
            return lastStatus_;
        }
        if (status == NodeStatus::Running)
        {
            lastStatus_ = NodeStatus::Running;
            return lastStatus_;
        }
        // Successなら次のノードへ
        ++currentIndex;
    }
    // 全てSuccess
    Reset();
    lastStatus_ = NodeStatus::Success;
    return lastStatus_;
}
