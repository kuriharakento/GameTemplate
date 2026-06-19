#include "SelectorNode.h"

NodeStatus SelectorNode::Tick(Blackboard& blackboard)
{
    while (currentIndex < children.size())
    {
        NodeStatus status = children[currentIndex]->Tick(blackboard);
        if (status == NodeStatus::Success)
        {
            Reset();
            lastStatus_ = NodeStatus::Success;
            return lastStatus_;
        }
        if (status == NodeStatus::Running)
        {
            lastStatus_ = NodeStatus::Running;
            return lastStatus_;
        }
        // Failureなら次のノードへ
        ++currentIndex;
    }
    // 全てFailure
    Reset();
    lastStatus_ = NodeStatus::Failure;
    return lastStatus_;
}
